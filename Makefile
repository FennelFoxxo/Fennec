BUILD_DIR=build/
SIMULATE_DIR=simulate/

SIMULATE_OPTIONS=-cpu Nehalem,-vme,-pdpe1gb,-xsave,-xsaveopt,-xsavec,-fsgsbase,-invpcid,+syscall,+lm,enforce -serial stdio -m size=300M -gdb tcp::1234
.PHONY: all clean simulate

all: $(BUILD_DIR)/disk.img

$(BUILD_DIR)/disk.img: $(BUILD_DIR)/build.ninja
	cmake --build $(BUILD_DIR)

$(BUILD_DIR)/build.ninja:
	mkdir -p build
	cd build && cmake -G Ninja -DKernelHugePage=OFF -DPLATFORM=x86_64 -DSIMULATION=TRUE ..

clean:
	cmake --build $(BUILD_DIR) --target clean

# Somehow the output of qemu can crash wsl, but filtering out non-ascii characters seems to fix it
simulate: $(BUILD_DIR)/disk.img
	qemu-system-x86_64 $(SIMULATE_OPTIONS) \
		-drive if=pflash,format=raw,readonly=on,file=${SIMULATE_DIR}/OVMF_CODE.fd \
		-drive format=raw,file=$(BUILD_DIR)/disk.img | tr -dc '[^ -~\012\015]'