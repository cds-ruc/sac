# SAC: A Co-Design Cache Algorithm for Emerging SMR-based High-Density Disks



## Preparation

### Hardware dependencies

- There is no dedicated hardware required if just to play with the SAC. We provide a pure emulation way including to emulate the cache and SMR device so that you can quickly see and compare the characteristic for each algorithm. However, you won't see any information of the I/O time. 

- If you want to verify the cache algorithm on real device, we suggest the hardware configuration as follow: 

  - The Seagate 8TB SMR drive model ST0008AS0002 is recommended. 

  - An SSD device is required to as the cache layer of the SMR, or use a memory file or Ramdisk instead, but it is recommended to be at least 40GiB. 

### Software dependencies

- The SAC project has been tested on CentOS Linux release 7.6.1810 (Core) based on Kernel 4.20.13-1 environment and is expected to run correctly in other Linux distributions. 

- FIO benchmark required for real SMR drive tests.

  

## Installation

### Source Code: 

Clone this project and goto the project root directory, then build the source code. 

```shell
git clone https://github.com/dcstrange/sac.git && cd sac
make
```



### DataSet

You have to download the trace files as workloads needed by the program. The trace files are available on the Google Drive, [Click here](https://drive.google.com/drive/folders/1zwZYwGB9PbuqAs3wlIE4cpIrYkdgUtYB). Put all the trace files into the project directory ``traces/``. (If the link fails, ju÷st let us know by Email: diansensun@gmail.com. )



