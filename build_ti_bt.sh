export ROOT=`pwd`
export KOBJ=`pwd`
export CROSS_COMPILE=arm-eabi-
export ARCH=arm
declare SW_DIR="drivers/bluetooth/compat-wireless-tibluez/"
cd drivers/bluetooth/compat-wireless-tibluez/
make KLIB=${KOBJ} KLIB_BUILD=${KOBJ} clean
make KLIB=${KOBJ} KLIB_BUILD=${KOBJ}
cd ${ROOT}
mkdir -p $ROOT/btimages
cp $ROOT/drivers/staging/ti-st/fm_drv.ko $ROOT/btimages/.
cp $ROOT/drivers/hid/hid-magicmouse.ko $ROOT/btimages/.
cp $SW_DIR/drivers/bluetooth/btwilink.ko $ROOT/btimages/.
cp $SW_DIR/net/bluetooth/rfcomm/rfcomm.ko $ROOT/btimages/.
cp $SW_DIR/net/bluetooth/bluetooth.ko $ROOT/btimages/.
cp $SW_DIR/net/bluetooth/bnep/bnep.ko $ROOT/btimages/.
cp $SW_DIR/net/bluetooth/hidp/hidp.ko $ROOT/btimages/.
cp $SW_DIR/net/bluetooth/hidp/hidp.ko $ROOT/btimages/.
