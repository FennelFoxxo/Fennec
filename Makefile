BUILD_DIR=build/
SIMULATE_DIR=simulate/
SHELL := /bin/bash

SIMULATE_OPTIONS=-cpu Nehalem,-vme,-pdpe1gb,-xsave,-xsaveopt,-xsavec,-fsgsbase,-invpcid,+syscall,+lm,enforce -serial stdio -m size=300M -gdb tcp::1234

REQUIRED_PACKAGES=make cmake ninja-build build-essential autoconf automake autopoint bison flex pkg-config libxml2-utils mtools python3-pip python3-virtualenv
REQUIRED_PYTHON_MODULES=ply Jinja2 lxml pyyaml

VENV_SOURCE_CMD=source .venv/bin/activate

.PHONY: build clean simulate install-packages

build: $(BUILD_DIR)/build.ninja
	$(VENV_SOURCE_CMD) && cmake --build $(BUILD_DIR)

$(BUILD_DIR)/build.ninja:
	mkdir -p build
	$(VENV_SOURCE_CMD) && cd build && cmake -G Ninja -DKernelHugePage=OFF -DPLATFORM=x86_64 -DSIMULATION=TRUE ..

clean:
	if [ -d "$(BUILD_DIR)" ]; then \
		cmake --build $(BUILD_DIR) --target clean; \
	fi

# Somehow the output of qemu can crash wsl, but filtering out non-ascii characters seems to fix it
simulate: build
	qemu-system-x86_64 $(SIMULATE_OPTIONS) \
		-drive if=pflash,format=raw,readonly=on,file=${SIMULATE_DIR}/OVMF_CODE.fd \
		-drive format=raw,file=$(BUILD_DIR)/disk.img | tr -dc '[^ -~\012\015]'