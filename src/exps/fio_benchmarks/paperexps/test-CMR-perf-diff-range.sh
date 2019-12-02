#!/bin/bash
#每次测试前需要确保SMR的Persistent Buffer(PB)是否已经被完全清空，若不是，请顺序写遍整个磁盘。

DEV_SMR=$1

echo "256M..."
DEVICE=${DEV_SMR} FILESIZE=256M BS=4K fio ./rdm_w_nobuf.fio > CMR-256M.log

echo "512M..."
DEVICE=${DEV_SMR} FILESIZE=512M BS=4K fio ./rdm_w_nobuf.fio > CMR-512M.log

echo "1G..."
DEVICE=${DEV_SMR} FILESIZE=1G BS=4K fio ./rdm_w_nobuf.fio > CMR-1G.log

echo "2G..."
DEVICE=${DEV_SMR} FILESIZE=2G BS=4K fio ./rdm_w_nobuf.fio > CMR-2G.log

echo "4G..."
DEVICE=${DEV_SMR} FILESIZE=4G BS=4K fio ./rdm_w_nobuf.fio > CMR-4G.log

echo "8G..."
DEVICE=${DEV_SMR} FILESIZE=8G BS=4K fio ./rdm_w_nobuf.fio > CMR-8G.log

echo "16G..."
DEVICE=${DEV_SMR} FILESIZE=16G BS=4K fio ./rdm_w_nobuf.fio > CMR-16G.log

echo "32G..."
DEVICE=${DEV_SMR} FILESIZE=32G BS=4K fio ./rdm_w_nobuf.fio > CMR-32G.log

echo "64G..."
DEVICE=${DEV_SMR} FILESIZE=64G BS=4K fio ./rdm_w_nobuf.fio > CMR-64G.log

echo "128G..."
DEVICE=${DEV_SMR} FILESIZE=128G BS=4K fio ./rdm_w_nobuf.fio > CMR-128G.log

echo "256G..."
DEVICE=${DEV_SMR} FILESIZE=256G BS=4K fio ./rdm_w_nobuf.fio > CMR-256G.log

echo "512G..."
DEVICE=${DEV_SMR} FILESIZE=512G BS=4K fio ./rdm_w_nobuf.fio > CMR-512G.log

echo "1T..."
DEVICE=${DEV_SMR} FILESIZE=1T BS=4K fio ./rdm_w_nobuf.fio > CMR-1T.log

echo "2T..."
DEVICE=${DEV_SMR} FILESIZE=2T BS=4K fio ./rdm_w_nobuf.fio > CMR-2T.log

echo "4T..."
DEVICE=${DEV_SMR} FILESIZE=4T BS=4K fio ./rdm_w_nobuf.fio > CMR-4T.log

echo "finish!"
