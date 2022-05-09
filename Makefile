all:
	@echo "Please precise make target : 'build' | 'flash' [_micro_speech | _magic_wand]"


clean:
	@echo Cleaning ...
	rm -vf *.bin


# MICRO SPEECH
build_micro_speech:
	$(MAKE) clean
	@echo Building ...

	# download all required dependencies and create the binary
	make -f tensorflow/lite/micro/tools/make/Makefile TARGET=sparkfun_edge micro_speech_bin

	# check successful binary creation
	@test -f \
		tensorflow/lite/micro/tools/make/gen/sparkfun_edge_cortex-m4/bin/micro_speech.bin && \
		echo "Binary was successfully created" || echo "Binary is missing"

	# set up some dummy cryptographic keys
	cp tensorflow/lite/micro/tools/make/downloads/AmbiqSuite-Rel2.2.0/tools/apollo3_scripts/keys_info0.py tensorflow/lite/micro/tools/make/downloads/AmbiqSuite-Rel2.2.0/tools/apollo3_scripts/keys_info.py

	# create a signed binary	
	python3 tensorflow/lite/micro/tools/make/downloads/AmbiqSuite-Rel2.2.0/tools/apollo3_scripts/create_cust_image_blob.py \
		--bin tensorflow/lite/micro/tools/make/gen/sparkfun_edge_cortex-m4/bin/micro_speech.bin \
		--load-address 0xC000 \
		--magic-num 0xCB \
		-o main_nonsecure_ota \
		--version 0x0

	# create flashable file
	python3 tensorflow/lite/micro/tools/make/downloads/AmbiqSuite-Rel2.2.0/tools/apollo3_scripts/create_cust_wireupdate_blob.py \
		--load-address 0x20000 \
		--bin main_nonsecure_ota.bin \
		-i 6 \
		-o main_nonsecure_wire \
		--options 0x1

	# should now have a file called main_nonsecure_wire.bin
	@test -f \
		main_nonsecure_wire.bin && \
		echo "Binary was successfully created" || echo "Binary is missing"


flash_micro_speech:
	$(MAKE) build_micro_speech
	@echo Flashing ...

	# connect board
	@read -p "Press ENTER when Sparfun is disconnected" OK
	ls /dev/tty*

	@read -p "Connect Sparkfun and press ENTER again" OK
	ls /dev/tty*

	@read -p "Enter full name of newly discovered device (default is '/dev/ttyUSB0'): " DEVICENAME; if [$$DEVICENAME = ""]; then export DEVICENAME=/dev/ttyUSB0; fi; \
	sudo chmod 777 $$DEVICENAME; \
	export BAUD_RATE=921600; \
	echo "Using BAUD_RATE=$$BAUD_RATE, DEVICENAME=$$DEVICENAME"; \
	read -p "Start holding button 14 ; Click button RST and then hit ENTER ; Hold on 14 until after seeing 'Sending Data Packet of length 8180'" OK; \
	python3 tensorflow/lite/micro/tools/make/downloads/AmbiqSuite-Rel2.2.0/tools/apollo3_scripts/uart_wired_update.py -b $$BAUD_RATE $$DEVICENAME -r 1 -f main_nonsecure_wire.bin -i 6


# MAGIC WAND
build_magic_wand:
	$(MAKE) clean
	@echo Building ...

	# download all required dependencies and create the binary
	make -f tensorflow/lite/micro/tools/make/Makefile TARGET=sparkfun_edge magic_wand_bin

	# check successful binary creation
	@test -f \
		tensorflow/lite/micro/tools/make/gen/sparkfun_edge_cortex-m4/bin/magic_wand.bin && \
		echo "Binary was successfully created" || echo "Binary is missing"

	# set up some dummy cryptographic keys
	cp tensorflow/lite/micro/tools/make/downloads/AmbiqSuite-Rel2.2.0/tools/apollo3_scripts/keys_info0.py tensorflow/lite/micro/tools/make/downloads/AmbiqSuite-Rel2.2.0/tools/apollo3_scripts/keys_info.py

	# create a signed binary	
	python3 tensorflow/lite/micro/tools/make/downloads/AmbiqSuite-Rel2.2.0/tools/apollo3_scripts/create_cust_image_blob.py \
		--bin tensorflow/lite/micro/tools/make/gen/sparkfun_edge_cortex-m4/bin/magic_wand.bin \
		--load-address 0xC000 \
		--magic-num 0xCB \
		-o main_nonsecure_ota \
		--version 0x0

	# create flashable file
	python3 tensorflow/lite/micro/tools/make/downloads/AmbiqSuite-Rel2.2.0/tools/apollo3_scripts/create_cust_wireupdate_blob.py \
		--load-address 0x20000 \
		--bin main_nonsecure_ota.bin \
		-i 6 \
		-o main_nonsecure_wire \
		--options 0x1

	# should now have a file called main_nonsecure_wire.bin
	@test -f \
		main_nonsecure_wire.bin && \
		echo "Binary was successfully created" || echo "Binary is missing"


flash_magic_wand:
	$(MAKE) build_magic_wand
	@echo Flashing ...

	# connect board
	@read -p "Press ENTER when Sparfun is disconnected" OK
	ls /dev/tty*

	@read -p "Connect Sparkfun and press ENTER again" OK
	ls /dev/tty*

	@read -p "Enter full name of newly discovered device (default is '/dev/ttyUSB0'): " DEVICENAME; if [$$DEVICENAME = ""]; then export DEVICENAME=/dev/ttyUSB0; fi; \
	sudo chmod 777 $$DEVICENAME; \
	export BAUD_RATE=921600; \
	echo "Using BAUD_RATE=$$BAUD_RATE, DEVICENAME=$$DEVICENAME"; \
	read -p "Start holding button 14 ; Click button RST and then hit ENTER ; Hold on 14 until after seeing 'Sending Data Packet of length 8180'" OK; \
	python3 tensorflow/lite/micro/tools/make/downloads/AmbiqSuite-Rel2.2.0/tools/apollo3_scripts/uart_wired_update.py -b $$BAUD_RATE $$DEVICENAME -r 1 -f main_nonsecure_wire.bin -i 6


monitor:
	@echo "To stop viewing the debug output, hit 'Ctrl+A', immediately followed by the 'K' key, then hit the 'Y' key."
	@read -p "Enter full name of device (default is '/dev/ttyUSB0'): " DEVICENAME; if [$$DEVICENAME = ""]; then export DEVICENAME=/dev/ttyUSB0; fi; \
	sudo chmod 777 $$DEVICENAME; \
	screen $$DEVICENAME 115200


build:
	$(MAKE) build_magic_wand


flash:
	$(MAKE) flash_magic_wand

quick_flash_and_monitor:
	$(MAKE) build_magic_wand

	@echo "To stop viewing the debug output, hit 'Ctrl+A', immediately followed by the 'K' key, then hit the 'Y' key."
	@read -p "Start holding button 14 ; Click button RST and then hit ENTER ; Hold on 14 until after seeing 'Sending Data Packet of length 8180'" OK; \
	export DEVICENAME=/dev/ttyUSB0; \
	sudo chmod 777 $$DEVICENAME; \
	export BAUD_RATE=921600; \
	echo "Using BAUD_RATE=$$BAUD_RATE, DEVICENAME=$$DEVICENAME"; \
	python3 tensorflow/lite/micro/tools/make/downloads/AmbiqSuite-Rel2.2.0/tools/apollo3_scripts/uart_wired_update.py -b $$BAUD_RATE $$DEVICENAME -r 1 -f main_nonsecure_wire.bin -i 6; \
	screen $$DEVICENAME 115200
