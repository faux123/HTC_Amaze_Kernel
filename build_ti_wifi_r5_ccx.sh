export ROOT=`pwd`
export KOBJ=`pwd`
export CROSS_COMPILE=arm-eabi-
export ARCH=arm
declare SW_DIR="drivers/net/wireless/compat-wireless-r5.00.09_ed_ccx/"

#make KLIB=${KOBJ} KLIB_BUILD=${KOBJ} clean
cd drivers/net/wireless/compat-wireless-r5.00.09_ed_ccx/
make KLIB=${KOBJ} KLIB_BUILD=${KOBJ} clean
make KLIB=${KOBJ} KLIB_BUILD=${KOBJ}

cd ${ROOT}

mkdir -p $ROOT/images
cp $SW_DIR/compat/compat.ko $ROOT/images/.
cp $SW_DIR/drivers/net/wireless/wl12xx/wl12xx.ko $ROOT/images/.
cp $SW_DIR/drivers/net/wireless/wl12xx/wl12xx_sdio.ko $ROOT/images/.
cp $SW_DIR/net/mac80211/mac80211.ko $ROOT/images/.
cp $SW_DIR/net/wireless/cfg80211.ko $ROOT/images/.
