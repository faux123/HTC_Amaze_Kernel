export ROOT=`pwd`
export KOBJ="/home/shawnjohnjr/repo2/kernel_msm-htc-3.0"
export CROSS_COMPILE=arm-eabi-
export ARCH=arm
make KLIB=${KOBJ} KLIB_BUILD=${KOBJ} clean
