# SAC: A Co-Design Cache Algorithm for Emerging SMR-based High-Density Disks
[![DOI](https://zenodo.org/badge/224646281.svg)](https://zenodo.org/badge/latestdoi/224646281)

SAC is a cache algorithm used in SSD-SMR hybrid storage systems. It has good performance in both read and write I / O mode. This project is a prototype of user-mode cache system used to verify the performance of different cache algorithms, in which we have 4 built-in algorithms -  SAC, LRU, MOST, MOST+CDC(a module of SAC). 

Follow this instruction, you will have a quick exploration of play-with-SAC!

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

You have to download the trace files as workloads needed by the program. The trace files are available on the Google Drive, [Click here](https://drive.google.com/drive/folders/1zwZYwGB9PbuqAs3wlIE4cpIrYkdgUtYB). Put all the trace files into the project directory ``traces/``. (If the link fails, just let us know by Email: diansensun@gmail.com. )



## Experiment 

### Options

It generates the binary file `sac` after build. Now you can play with the SAC. The following table describes the option argument you may or have to specify. With different options, you will run the cases including SMR emulator, no real I/O, small or big trace, read or write only mode, etc. 

Before the test, you need to ensure that the SSD and SMR device files are present, and set the files path to the option `--cache-dev` and `--smr-dev`. 
		If you don't have the SMR device, you still can use a regular HDD device file instead and use the SMR emulator module. Please for sure you have the permission to access the files, otherwise, you will get the error information like `errno:13`. 


| Options           | Need Arg? | Arguments                                                    | Default |
| ----------------- | --------- | ------------------------------------------------------------ | ------- |
| `--cache-dev`     | ✅         | Device file path of the ssd/ramdisk for cache layer.         | NULL    |
| `--smr-dev`       | ✅         | Device file path of the SMR drive or HDD for SMR emulator.   | NULL    |
| `--algorithm`     | ✅         | One of [SAC], [LRU], [MOST], [MOST+CDC]                       | SAC     |
| `--no-cache`      | ❌         |                                                              |         |
| `--use-emulator`  | ❌         |                                                              |         |
| `--no-real-io`    | ❌         |                                                              |         |
| `--workload`      | ✅         | Workload number for [1~11] corresponding to different trace file. | -1      |
| `--workload-file` | ✅         | Or you can specofy the trace file path manually.             | NULL    |
| `--workload-mode` | ✅         | Three workload mode: [**R**]:read-only, [**W**]:write-only, [**RW**]:read-write | RW      |
| `--cache-size`    | ✅         | Cache size: [size]{+M,G}. E.g. 32G                           | 32GB    |
| `--pb-size`       | ✅         | SMR Persistent Buffer size: [size]{+M,G}. E.g. 600M          | 600MB   |
| `--offset`        | ✅         | Start LBA offset of the SMR: [size]{+M,G}. E.g. 1G           | 0       |
| `--requests`      | ✅         | Requst number you want to run: [Nunmber].                    | -1      |
| `--help`          | ✅         | Show Help Manual.                    |       |
### Examples

#### Example 1: Quick Run!

```shell
./sac --algorithm SAC --workload 11 --use-emulator --no-real-io --requests 125000000
./sac --algorithm LRU --workload 11 --use-emulator --no-real-io --requests 125000000
./sac --algorithm MOST --workload 11 --use-emulator --no-real-io --requests 125000000
```

This command let you quickly verify and compare the effectiveness of a cache algorithm, and its performance will be reflected from the results of the SMR emulator where the most critical indicator is RMW trigger count. Note that, this command will not generate the real disk I/O due to the option `--no-real-io` which discards the request before it is sent to the device, while the metadata structure of the cache algorithm and emulator still running in memory. 
In this case, your test environment does not need to deploy the SSD and the SMR devices,  nor does it need to specify the `--cache-dev` and `--smr-dev`. 

Also, we provide other comparison cache algorithms including LRU, MOST, and MOST with CDC. Change the `--algorithm` option to test other algorithms, such as `--algorithm LRU`. 

Or, you can just run the script `scripts/quicktest_compare_algorithms.sh` for a quick comparason test among LRU, MOST, SAC in write-only mode, run:

```shell
cd scripts && ./quicktest_compare_algorithms.sh
```

The outputs can be found in `logs/`. 

#### Example 2: Big dataset and real disk I/O

```shell
./sac --cache-dev [FILE] --smr-dev [FILE] --workload 11 --algorithm SAC --requests 100000000
```

For the typical test on SSD-SMR hybrid storage, run the this command which will run the workload number 11 (which is a big dataset) for 100 million requests in read and write mix mode (default), and using the SAC cache algorithm (default). Note that, the real disk I/O will cost a long time, approximately 10+ hours for SAC algorithm, and much more for other algorithms. 

#### Example 3: Small dataset and write-only mode

```shell
./sac --cache-dev [FILE] --smr-dev [FILE] --algorithm SAC --workload 5 --workload-mode W
```

If you need the workload running in write-only mode, you should add the option `--workload-mode W` which will only execute write requests in the trace file. 

There are three optional value of `--workload-mode`, they are `W` for write-only mode, `R`for read-only mode, and `RW` for read-write mix mode. 

#### Example 4: Use emulator instead of SMR drive

```shell
./sac --cache-dev [FILE] --smr-dev [FILE] --workload 5 --workload-mode W --use-emulator
```

We enable you to verify the SAC and other cache algorithms without a SMR drive. With the option `--use-emulator`, the program will emulate the behavior of the STL (Shingle Translation Layer) on the regular HDD you specify. The SMR emulator module will then output the information about the I/O time, write amplification, RMW counts, etc. 

Note that, if you use the option `--use-emulator` without `--no-real-io`, the program will use the HDD device specified by `--smr-dev` to execute the behavior of STL and generate the I/O. In this case, you must to specify a real HDD. 

### Note!
Before testing the real SMR drive, you need to force clean the PB area of the SMR, otherwise the remaining data will affect the performance of the next test. We provide the PB cleaning script in the project scripts/smr-pb-forceclean.sh, run 

```shell 
./smr-pb-forceclean.sh [SMR FILE]
```

But be careful NOT to use this script in any production environment, it will overwrite the data of the `[SMR FILE]`. 

### Contact: 

Author: Sun, Diansen and Chai, Yunpeng

Affiliation: Renmin University of China 

Email: diansensun@gmail.com
