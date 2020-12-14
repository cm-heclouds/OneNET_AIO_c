#!/bin/bash
echo "Load Configuration:"
config_files=`find ./build/configs -name *.config -type f`
config_list=($config_files)
index=1
for i in $config_files
do
    echo "$index. $(basename $i .config)"
    index=`expr $index + 1`
done
echo "$index. Custom Configuration"
read -p "Input your choice: " config_index
if [ $config_index -ne $index ]; then
    ./build/Kconfiglib/defconfig.py --kconfig Config.menu ${config_list[`expr $config_index - 1`]}
fi
./build/Kconfiglib/menuconfig.py Config.menu
./build/Kconfiglib/genconfig.py Config.menu