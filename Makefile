BUILD_DIR=build/
SIMULATE_DIR=simulate/
SHELL := /bin/bash

SIMULATE_OPTIONS=-cpu Nehalem,-vme,-pdpe1gb,-xsave,-xsaveopt,-xsavec,-fsgsbase,-invpcid,+syscall,+lm,enforce -serial stdio -m size=300M -gdb tcp::1234

VENV_SOURCE_CMD=source .venv/bin/activate

.PHONY: build clean simulate

build: $(BUILD_DIR)/build.ninja
	$(VENV_SOURCE_CMD) && cmake --build $(BUILD_DIR)

$(BUILD_DIR)/build.ninja:
	mkdir -p build
	$(VENV_SOURCE_CMD) && cd build && cmake -G Ninja -DKernelHugePage=OFF -DPLATFORM=x86_64 -DSIMULATION=TRUE ..

clean:
	if [ -d "$(BUILD_DIR)" ]; then \
		cmake --build $(BUILD_DIR) --target clean; \
	fi

simulate: build
	qemu-system-x86_64 $(SIMULATE_OPTIONS) \
		-drive if=pflash,format=raw,readonly=on,file=${SIMULATE_DIR}/OVMF_CODE.fd \
		-drive format=raw,file=$(BUILD_DIR)/disk.img