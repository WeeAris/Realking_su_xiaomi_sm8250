#!/bin/bash
#set -e
## Copy this script inside the kernel directory
KERNEL_DEFCONFIG=vendor/apollo_defconfig
export PATH="/root/rohail33/kernel/clang/bin:$PATH"
export ARCH=arm64
export SUBARCH=arm64
export KBUILD_COMPILER_STRING="$(/root/rohail33/kernel/clang/bin/clang --version | head -n 1 | perl -pe 's/\(http.*?\)//gs' | sed -e 's/  */ /g' -e 's/[[:space:]]*$//')"

if ! [ -d "/root/rohail33/kernel/clang" ]; then
echo "Proton clang not found! Cloning..."
if ! git clone -q https://gitlab.com/ZyCromerZ/clang.git --depth=1 --single-branch /home/rohail/kernel/proton-clang; then
echo "Cloning failed! Aborting..."
exit 1
fi
fi
KERNEL_DIR=`pwd`
ZIMAGE_DIR="$KERNEL_DIR/out/arch/arm64/boot"
# Speed up build process
MAKE="./makeparallel"
BUILD_START=$(date +"%s")
blue='\033[0;34m'
cyan='\033[0;36m'
yellow='\033[0;33m'
red='\033[0;31m'
nocol='\033[0m'

echo "**** Kernel defconfig is set to $KERNEL_DEFCONFIG ****"
echo -e "$blue***********************************************"
echo "          BUILDING KERNEL          "
echo -e "***********************************************$nocol"
make $KERNEL_DEFCONFIG O=out CC=clang
make -j$(nproc --all) O=out \
                      ARCH=arm64 \
                      CROSS_COMPILE=aarch64-linux-gnu- \
                      CROSS_COMPILE_COMPAT=arm-linux-gnueabi- \
                      LLVM=1 \
                      LLVM_IAS=1

TIME="$(date "+%Y%m%d-%H%M%S")"
mkdir -p tmp
cp -fp $ZIMAGE_DIR/Image.gz tmp
cp -rp ./anykernel/* tmp
cd tmp
7za a -mx9 tmp.zip *
cd ..
rm *.zip
cp -fp tmp/tmp.zip RealKing-Alioth-AOSP-$TIME.zip
rm -rf tmp
echo $TIME
