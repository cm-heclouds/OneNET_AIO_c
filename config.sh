#!/bin/bash
echo "Load Configuration:"
if [ -d "./build/configs" ]; then
    config_files=`find ./build/configs -name *.config -type f`
fi
config_list=($config_files)
index=1
for i in $config_files
do
    echo "$index. $(basename $i .config)"
    index=`expr $index + 1`
done
echo "$index. Custom Configuration"
read -p "Input your choice: " config_index
if [ -n "$config_index" ]; then
    if [ $config_index -ne $index ]; then
        ./build/Kconfiglib/defconfig.py --kconfig Config.menu ${config_list[`expr $config_index - 1`]}
    fi
fi
./build/Kconfiglib/menuconfig.py Config.menu
./build/Kconfiglib/genconfig.py Config.menu