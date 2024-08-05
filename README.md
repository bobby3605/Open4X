# Open4X
A data-driven game engine written in c++ and vulkan from scratch.
Currently, this is for my own learning and fun, it is not practically useful
The rewrite branch has the latest updates, but is not yet feature complete with master, so it hasn't been merged yet.
![1 million cubes, PBR, and some test models on a 7900xtx](https://github.com/user-attachments/assets/dee4710a-6525-493c-a94b-3525ebbe4f0b)

## Building on Linux:
First run 'git submodule update --init --recursive'
Just run 'make' in the root directory. This is using 'make' and cmake to minimize the number of commands I need to use to build and run it.

'make run' will run open4x.

## Settings:
assets/settings.json is the configuration file. You can edit the number of randomly positioned Box.glb models and the position limit, along with some miscellaneous settings. 
