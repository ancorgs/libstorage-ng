
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE libstorage

#include <boost/algorithm/string.hpp>
#include <boost/test/unit_test.hpp>

#include "storage/Devices/Disk.h"
#include "storage/Devices/Msdos.h"
#include "storage/Devices/Gpt.h"
#include "storage/Devices/Partition.h"
#include "storage/Devices/LvmVg.h"
#include "storage/Devices/LvmLv.h"
#include "storage/Devices/Luks.h"
#include "storage/Devicegraph.h"
#include "storage/Storage.h"
#include "storage/Environment.h"
#include "storage/Utils/Region.h"
#include "storage/Utils/HumanString.h"
#include "storage/FreeInfo.h"


using namespace std;
using namespace storage;


const unsigned long long spg = GiB / 512;	// sectors per GiB


BOOST_AUTO_TEST_CASE(test_msdos)
{
    set_logger(get_stdout_logger());

    Environment environment(true, ProbeMode::NONE, TargetMode::DIRECT);

    Storage storage(environment);

    Devicegraph* staging = storage.get_staging();

    Disk* sda = Disk::create(staging, "/dev/sda", Region(0, 160 * spg, 512));

    PartitionTable* msdos = sda->create_partition_table(PtType::MSDOS);

    Partition* sda1 = msdos->create_partition("/dev/sda1", Region(10 * spg, 10 * spg, 512), PartitionType::PRIMARY);

    Partition* sda2 = msdos->create_partition("/dev/sda2", Region(40 * spg, 20 * spg, 512), PartitionType::PRIMARY);
    sda2->create_blk_filesystem(FsType::EXT4);

    Partition* sda3 = msdos->create_partition("/dev/sda3", Region(80 * spg, 40 * spg, 512), PartitionType::PRIMARY);
    sda3->create_blk_filesystem(FsType::SWAP);

    {
	ResizeInfo resize_info = sda1->detect_resize_info();
	BOOST_CHECK(resize_info.resize_ok);
	BOOST_CHECK_EQUAL(resize_info.min_size, 512);
	BOOST_CHECK_EQUAL(resize_info.max_size, 30 * GiB);
    }

    {
	ResizeInfo resize_info = sda2->detect_resize_info();
	BOOST_CHECK(resize_info.resize_ok);
	BOOST_CHECK_EQUAL(resize_info.min_size, 32 * MiB);
	BOOST_CHECK_EQUAL(resize_info.max_size, 40 * GiB);
    }

    {
	ResizeInfo resize_info = sda3->detect_resize_info();
	BOOST_CHECK(resize_info.resize_ok);
	BOOST_CHECK_EQUAL(resize_info.min_size, 40 * KiB);
	BOOST_CHECK_EQUAL(resize_info.max_size, 80 * GiB);
    }
}


BOOST_AUTO_TEST_CASE(test_gpt)
{
    set_logger(get_stdout_logger());

    Environment environment(true, ProbeMode::NONE, TargetMode::DIRECT);

    Storage storage(environment);

    Devicegraph* staging = storage.get_staging();

    Disk* sda = Disk::create(staging, "/dev/sda", Region(0, 160 * spg, 512));

    PartitionTable* gpt = sda->create_partition_table(PtType::GPT);

    Partition* sda1 = gpt->create_partition("/dev/sda1", Region(10 * spg, 10 * spg, 512), PartitionType::PRIMARY);

    LvmVg* lvm_vg = LvmVg::create(staging, "vg");
    lvm_vg->add_lvm_pv(sda1);

    LvmLv* lvm_lv1 = lvm_vg->create_lvm_lv("lv1", LvType::NORMAL, 1 * GiB);

    LvmLv* lvm_lv2 = lvm_vg->create_lvm_lv("lv2", LvType::NORMAL, 1 * GiB);
    lvm_lv2->create_blk_filesystem(FsType::EXT4);

    Partition* sda2 = gpt->create_partition("/dev/sda2", Region(30 * spg, 10 * spg, 512), PartitionType::PRIMARY);

    Encryption* encryption = sda2->create_encryption("cr-test1");
    encryption->create_blk_filesystem(FsType::EXT4);

    Partition* sda3 = gpt->create_partition("/dev/sda3", Region(50 * spg, 10 * spg, 512), PartitionType::PRIMARY);

    {
	ResizeInfo resize_info = sda1->detect_resize_info();
	BOOST_CHECK(resize_info.resize_ok);
	BOOST_CHECK_EQUAL(resize_info.min_size, 5 * MiB);
	BOOST_CHECK_EQUAL(resize_info.max_size, 20 * GiB);
    }

    {
	ResizeInfo resize_info = lvm_lv1->detect_resize_info();
	BOOST_CHECK(resize_info.resize_ok);
	BOOST_CHECK_EQUAL(resize_info.min_size, 4 * MiB);
	BOOST_CHECK_EQUAL(resize_info.max_size, 9 * GiB - 4 * MiB);
    }

    {
	ResizeInfo resize_info = lvm_lv2->detect_resize_info();
	BOOST_CHECK(resize_info.resize_ok);
	BOOST_CHECK_EQUAL(resize_info.min_size, 32 * MiB);
	BOOST_CHECK_EQUAL(resize_info.max_size, 9 * GiB - 4 * MiB);
    }

    {
	ResizeInfo resize_info = sda2->detect_resize_info();
	BOOST_CHECK(resize_info.resize_ok);
	BOOST_CHECK_EQUAL(resize_info.min_size, 34 * MiB);
	BOOST_CHECK_EQUAL(resize_info.max_size, 20 * GiB);
    }

    {
	ResizeInfo resize_info = sda3->detect_resize_info();
	BOOST_CHECK(resize_info.resize_ok);
	BOOST_CHECK_EQUAL(resize_info.min_size, 512);
	BOOST_CHECK_EQUAL(resize_info.max_size, 110 * GiB - 16.5 * KiB);
    }
}
