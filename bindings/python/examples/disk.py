#!/usr/bin/python

from storage import DeviceGraph, Disk, BlkDevice, GPT, EXT4, SWAP


device_graph = DeviceGraph()

sda = Disk.create(device_graph, "/dev/sda")

gpt = sda.createPartitionTable(GPT)

sda1 = gpt.createPartition("/dev/sda1")
sda2 = gpt.createPartition("/dev/sda2")

ext4 = sda1.createFilesystem(EXT4)
swap = sda2.createFilesystem(SWAP)

device_graph.print_graph()


print "partitions on gpt:"
for partition in gpt.getPartitions():
  print "  %s %s" % (partition.getDisplayName(), partition.getNumber())
print


print "descendants of sda:"
for device in sda.getDescendants(False):
  print "  %s" % device.getDisplayName()
print


tmp1 = BlkDevice.find(device_graph, "/dev/sda1")
print tmp1.getDisplayName()
