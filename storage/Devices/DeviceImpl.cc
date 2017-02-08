/*
 * Copyright (c) [2014-2015] Novell, Inc.
 * Copyright (c) 2016 SUSE LLC
 *
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, contact Novell, Inc.
 *
 * To contact Novell about this file by physical or electronic mail, you may
 * find current contact information at www.novell.com.
 */


#include "storage/Devices/DeviceImpl.h"
#include "storage/Devicegraph.h"
#include "storage/Action.h"
#include "storage/Utils/XmlFile.h"
#include "storage/Utils/StorageTmpl.h"
#include "storage/FreeInfo.h"
#include "storage/Storage.h"


namespace storage
{

    using namespace std;


    const char* DeviceTraits<Device>::classname = "Device";


    sid_t Device::Impl::global_sid = 42;	// just a random number ;)


    Device::Impl::Impl()
	: sid(global_sid++), devicegraph(nullptr), vertex(), userdata()
    {
    }


    Device::Impl::Impl(const xmlNode* node)
	: sid(0), devicegraph(nullptr), vertex(), userdata()
    {
	if (!getChildValue(node, "sid", sid))
	    ST_THROW(Exception("no sid"));
    }


    bool
    Device::Impl::operator==(const Impl& rhs) const
    {
	if (typeid(*this) != typeid(rhs))
	    return false;

	return equal(rhs);
    }


    bool
    Device::Impl::exists_in_devicegraph(const Devicegraph* devicegraph) const
    {
	return devicegraph->device_exists(sid);
    }


    bool
    Device::Impl::exists_in_probed() const
    {
	return exists_in_devicegraph(get_storage()->get_probed());
    }


    bool
    Device::Impl::exists_in_staging() const
    {
	return exists_in_devicegraph(get_storage()->get_staging());
    }


    const Storage*
    Device::Impl::get_storage() const
    {
	return get_devicegraph()->get_storage();
    }


    void
    Device::Impl::probe_pass_1(Devicegraph* probed, SystemInfo& systeminfo)
    {
    }


    void
    Device::Impl::probe_pass_2(Devicegraph* probed, SystemInfo& systeminfo)
    {
    }


    void
    Device::Impl::save(xmlNode* node) const
    {
	setChildValue(node, "sid", sid);
    }


    void
    Device::Impl::check() const
    {
    }


    void
    Device::Impl::set_devicegraph_and_vertex(Devicegraph* devicegraph,
					     Devicegraph::Impl::vertex_descriptor vertex)
    {
	Impl::devicegraph = devicegraph;
	Impl::vertex = vertex;

	const Device* device = devicegraph->get_impl()[vertex];
	if (&device->get_impl() != this)
	    ST_THROW(LogicException("wrong vertex for back references"));
    }


    Devicegraph*
    Device::Impl::get_devicegraph()
    {
	if (!devicegraph)
	    ST_THROW(LogicException("not part of a devicegraph"));

	return devicegraph;
    }


    const Devicegraph*
    Device::Impl::get_devicegraph() const
    {
	if (!devicegraph)
	    ST_THROW(LogicException("not part of a devicegraph"));

	return devicegraph;
    }


    Devicegraph::Impl::vertex_descriptor
    Device::Impl::get_vertex() const
    {
	if (!devicegraph)
	    ST_THROW(LogicException("not part of a devicegraph"));

	return vertex;
    }


    bool
    Device::Impl::has_children() const
    {
	return devicegraph->get_impl().num_children(vertex) > 0;
    }


    size_t
    Device::Impl::num_children() const
    {
	return devicegraph->get_impl().num_children(vertex);
    }


    bool
    Device::Impl::has_parents() const
    {
	return devicegraph->get_impl().num_parents(vertex) > 0;
    }


    size_t
    Device::Impl::num_parents() const
    {
	return devicegraph->get_impl().num_parents(vertex);
    }


    void
    Device::Impl::remove_descendants()
    {
	Devicegraph::Impl& devicegraph_impl = devicegraph->get_impl();

	for (Devicegraph::Impl::vertex_descriptor descendant : devicegraph_impl.descendants(vertex, false))
	    devicegraph_impl.remove_vertex(descendant);
    }


    ResizeInfo
    Device::Impl::detect_resize_info() const
    {
	return ResizeInfo(false);
    }


    void
    Device::Impl::parent_has_new_region(const Device* parent)
    {
    }


    void
    Device::Impl::add_create_actions(Actiongraph::Impl& actiongraph) const
    {
	vector<Action::Base*> actions;

	actions.push_back(new Action::Create(sid));

	actiongraph.add_chain(actions);
    }


    void
    Device::Impl::add_modify_actions(Actiongraph::Impl& actiongraph, const Device* lhs) const
    {
	add_reallot_actions(actiongraph, lhs);
    }


    void
    Device::Impl::add_reallot_actions(Actiongraph::Impl& actiongraph, const Device* lhs) const
    {
	set<sid_t> lhs_sids;
	for (const Device* device : lhs->get_parents())
	    lhs_sids.insert(device->get_sid());

	set<sid_t> rhs_sids;
	for (const Device* device : get_device()->get_parents())
	    rhs_sids.insert(device->get_sid());

	vector<sid_t> added_sids;
	set_difference(rhs_sids.begin(), rhs_sids.end(), lhs_sids.begin(), lhs_sids.end(),
		       back_inserter(added_sids));

	vector<sid_t> removed_sids;
	set_difference(lhs_sids.begin(), lhs_sids.end(), rhs_sids.begin(), rhs_sids.end(),
		       back_inserter(removed_sids));

	for (sid_t sid : added_sids)
	{
	    const Device* device = actiongraph.get_devicegraph(RHS)->find_device(sid);
	    actiongraph.add_vertex(new Action::Reallot(get_sid(), ReallotMode::EXTEND, device));
	}

	for (sid_t sid : removed_sids)
	{
	    const Device* device = actiongraph.get_devicegraph(LHS)->find_device(sid);
	    actiongraph.add_vertex(new Action::Reallot(get_sid(), ReallotMode::REDUCE, device));
	}
    }


    void
    Device::Impl::add_delete_actions(Actiongraph::Impl& actiongraph) const
    {
	vector<Action::Base*> actions;

	actions.push_back(new Action::Delete(sid));

	actiongraph.add_chain(actions);
    }


    bool
    Device::Impl::equal(const Impl& rhs) const
    {
	return sid == rhs.sid;
    }


    void
    Device::Impl::log_diff(std::ostream& log, const Impl& rhs) const
    {
	storage::log_diff(log, "sid", sid, rhs.sid);
    }


    void
    Device::Impl::print(std::ostream& out) const
    {
	out << get_classname() << " sid:" << get_sid()
	    << " displayname:" << get_displayname();
    }


    Text
    Device::Impl::do_create_text(Tense tense) const
    {
	return UntranslatedText("error: stub do_create_text called");
    }


    void
    Device::Impl::do_create() const
    {
	ST_THROW(LogicException("stub do_create called"));
    }


    Text
    Device::Impl::do_delete_text(Tense tense) const
    {
	return UntranslatedText("error: stub do_delete_text called");
    }


    void
    Device::Impl::do_delete() const
    {
	ST_THROW(LogicException("stub do_delete called"));
    }


    Text
    Device::Impl::do_activate_text(Tense tense) const
    {
	return UntranslatedText("error: stub do_activate_text called");
    }


    void
    Device::Impl::do_activate() const
    {
	ST_THROW(LogicException("stub do_activate called"));
    }


    Text
    Device::Impl::do_deactivate_text(Tense tense) const
    {
	return UntranslatedText("error: stub do_deactivate_text called");
    }


    void
    Device::Impl::do_deactivate() const
    {
	ST_THROW(LogicException("stub do_deactivate called"));
    }


    Text
    Device::Impl::do_resize_text(ResizeMode resize_mode, const Device* lhs, const Device* rhs,
				 Tense tense) const
    {
	return UntranslatedText("error: stub do_resize_text called");
    }


    void
    Device::Impl::do_resize(ResizeMode resize_mode) const
    {
	ST_THROW(LogicException("stub do_resize called"));
    }


    Text
    Device::Impl::do_reallot_text(ReallotMode reallot_mode, const Device* device, Tense tense) const
    {
	return UntranslatedText("error: stub do_reallot_text called");
    }


    void
    Device::Impl::do_reallot(ReallotMode reallot_mode, const Device* device) const
    {
	ST_THROW(LogicException("stub do_reallot called"));
    }


    namespace Action
    {

	Text
	Activate::text(const Actiongraph::Impl& actiongraph, Tense tense) const
	{
	    const Device* device = get_device(actiongraph, RHS);
	    return device->get_impl().do_activate_text(tense);
	}


	void
	Activate::commit(const Actiongraph::Impl& actiongraph) const
	{
	    const Device* device = get_device(actiongraph, RHS);
	    device->get_impl().do_activate();
	}


	Text
	Deactivate::text(const Actiongraph::Impl& actiongraph, Tense tense) const
	{
	    const Device* device = get_device(actiongraph, LHS);
	    return device->get_impl().do_deactivate_text(tense);
	}


	void
	Deactivate::commit(const Actiongraph::Impl& actiongraph) const
	{
	    const Device* device = get_device(actiongraph, LHS);
	    device->get_impl().do_deactivate();
	}


	Text
	Resize::text(const Actiongraph::Impl& actiongraph, Tense tense) const
	{
	    const Device* device_lhs = get_device(actiongraph, LHS);
	    const Device* device_rhs = get_device(actiongraph, RHS);

	    const Device* device = get_device(actiongraph, get_side());
	    return device->get_impl().do_resize_text(resize_mode, device_lhs, device_rhs, tense);
	}


	void
	Resize::commit(const Actiongraph::Impl& actiongraph) const
	{
	    const Device* device = get_device(actiongraph, get_side());
	    device->get_impl().do_resize(resize_mode);
	}


	void
	Resize::add_dependencies(Actiongraph::Impl::vertex_descriptor vertex,
				 Actiongraph::Impl& actiongraph) const
	{
	    Modify::add_dependencies(vertex, actiongraph);

	    // Add dependencies to resize actions of children.

	    vector<sid_t> sid_children;

	    for (const Device* child : get_device(actiongraph, RHS)->get_children())
		sid_children.push_back(child->get_sid());

	    for (Actiongraph::Impl::vertex_descriptor other_vertex : actiongraph.vertices())
	    {
		const Action::Resize* other_resize_action = dynamic_cast<const Action::Resize*>(actiongraph[other_vertex]);
		if (other_resize_action && contains(sid_children, other_resize_action->sid))
		{
		    if (resize_mode == ResizeMode::SHRINK)
			actiongraph.add_edge(other_vertex, vertex);
		    else
			actiongraph.add_edge(vertex, other_vertex);
		}
	    }
	}


	Text
	Reallot::text(const Actiongraph::Impl& actiongraph, Tense tense) const
	{
	    const Device* device_rhs = get_device(actiongraph, RHS);
	    return device_rhs->get_impl().do_reallot_text(reallot_mode, device, tense);
	}


	void
	Reallot::commit(const Actiongraph::Impl& actiongraph) const
	{
	    const Device* device_rhs = get_device(actiongraph, RHS);
	    device_rhs->get_impl().do_reallot(reallot_mode, device);
	}


	void
	Reallot::add_dependencies(Actiongraph::Impl::vertex_descriptor vertex,
				  Actiongraph::Impl& actiongraph) const
	{
	    Modify::add_dependencies(vertex, actiongraph);

	    if (reallot_mode == ReallotMode::REDUCE)
	    {
		// Make sure the device (PV) is removed before being destroyed
		for (Actiongraph::Impl::vertex_descriptor tmp :
			 actiongraph.actions_with_sid(device->get_sid(), ONLY_FIRST))
		    actiongraph.add_edge(vertex, tmp);
	    }
	    else
	    {
		// Make sure the device is created before being added
		for (Actiongraph::Impl::vertex_descriptor tmp :
			 actiongraph.actions_with_sid(device->get_sid(), ONLY_LAST))
		    actiongraph.add_edge(tmp, vertex);

		// If the device was assigned elsewhere, make sure it's removed
		// from there before being re-assigned
		for (Actiongraph::Impl::vertex_descriptor tmp : actiongraph.vertices())
		    if (action_removes_device(actiongraph[tmp]))
		    {
			actiongraph.add_edge(tmp, vertex);
			break;
		    }
	    }
	}


	bool
	Reallot::action_removes_device(const Action::Base* action) const
	{
	    const Action::Reallot* reallot;

	    reallot = dynamic_cast<const Action::Reallot*>(action);
	    if (!reallot) return false;
	    if (reallot->reallot_mode != ReallotMode::REDUCE) return false;
	    return (reallot->device->get_sid() == device->get_sid());
	}

    }

}
