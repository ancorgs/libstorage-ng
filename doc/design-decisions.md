
Design Decisions
================


C++11
-----

Here are some reasons for using C++11 for libstorage. Of course some reasons
also apply to other programming languages.

- Allows reuse of existing code from libstorage.

- Widely used programming language
  (http://www.tiobe.com/index.php/content/paperinfo/tpci/index.html). Even
  several compilers are available (e.g. gcc-c++ and clang).

- Language is standardized by ISO. One strong focus is compatibility.

- Compiler performs many checks on code (starting with type-checks) at
  compile-time. Additionally tools for static code analysis are available
  (e.g. https://scan.coverity.com/).

- Comprehensive and documented graph library available
  (http://www.boost.org/doc/libs/1_59_0/libs/graph/doc/index.html).

- Wide range of libraries and tools (e.g. http://www.swig.org/ - a generator
  for scripting language bindings for e.g. Python, Ruby and Perl) are
  available and included in many distributions like openSUSE, Fedora, SLE and
  RHEL.

- The developers are confident with it.


Query Type of Objects in Device-Graph
-------------------------------------

There are no functions in Device and Holder to query the type,
e.g. ```device.is_disk()```.

Such an interface would be very bad idea since for every new class the base
class would have to be modified. With such an interface ABI stability is not
possible.

Instead use ```is_disk(device)```.


Pimpl Idiom
-----------

This is widely discussed including benefits and drawbacks so no general
discussion is needed here. Esp. important is:

- It allows hiding many implementation details including the boost graph
  classes from the public interface.


Backreference of Objects in Device-Graph
----------------------------------------

The device and holder objects stored in the device-graph have a backreference
to the device-graph and thus cannot be used outside of a device-graph.

The backreference is needed by many functions of the device objects, e.g. the
function of a logical volume need the physical extent size of the volume group
to transform from size in bytes to size in extents and vice versa.

Without the backreference many functions would need two extra parameters, the
device-graph and the vertex_descriptor (which is not even part of the public
interface). That would be a very cumbersome to use API and most functions
could still not be used without the device-graph.


No Global Find Function
-----------------------

So far there is no global find function. There have been some proposals, e.g.

```
f = Storage.find(device_graph, "filesystem");
f.add_filter("mountpoint", "/");
f.first();
```

Such an interface has several drawbacks:

1. The API converts compile-time checks to runtime errors, e.g. if you search
   for a flag that does not exist.

   If you replace the strings by enums you have a central point that has to be
   modified for each new class and flag. That would ruin ABI stability.

2. It's not type-safe in that f.first() cannot return a Filesystem object but
   only a Device. So manual casting is needed.


Language Bindings
-----------------

The language bindings are all automatically generated by swig and closely
match the C++ API. Only some general modifications are done, e.g. in Ruby
getter and setter functions are renamed.

This has the consequence that the bindings do not look so natural in the
target languages. Some developers requested more ruby-like bindings.

But that adds a lot extra work: For the target languages bindings have to be
maintained, documented and thorough tested. Additional it will be more
demanding for developers to use libstorage with different languages and it
could complicate debugging.


No use of BGL subgraph and filtered_graph in the API
----------------------------------------------------

The BGL subgraph class implements induced subgraphs. Modifying a subgraph also
modifies the parent graphs.

Subgraphs could be used to group device objects, e.g. a subgraph for each disk
and its partitions and a subgraph for each volume group and its logical
volumes. But we do not see advantages of such a design, quite the contrary it
complicate creating edges from inside those subgraph to other nodes.

The BGL filtered_graph is intended to allow using standard graph algorithms on
filtered graphs.

So far all the examples people brought forward to require some filtering were
not dealing with filtered graphs but only with lists of devices/nodes. The
holders/edges were never important. So simple functions returning lists of
devices are sufficient and using filtered_graph is not needed.

Technical the filtering done by filtered_graph is not done once during
creation of the filtered_graph but whenever the filtered graph is
accessed. Thus iterating twice over a filtered graph calls also the vertex
predicate function twice. Having a complex predicate function that
e.g. inspects the graph structure and not only the data of the single vertex
can easily cause quadratic runtime complexity.

So overall we do not see justification to invest in providing filtered_graph
in the API. Internally it should be easy to use it when a use cases are found.
