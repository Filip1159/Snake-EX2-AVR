# Snake EX2 AVR — host checks and firmware build.
#
# `make test` runs the complete host suite, including the Q2 asset golden test.
# `make firmware` builds the ATmega644PA image.  `make assets` regenerates the
# checked-in assets from reference/d; never hand-edit assets_gen.*.

.PHONY: all test assets assets-test visual firmware upload fuses fuses-read \
        stack-measure stack-measure-upload clean

AVRDUDE ?= avrdude
USBASP_PORT ?= usb
# As-built internal 8 MHz RC.  Use LFUSE=0xF7 only with an external 8 MHz crystal.
LFUSE ?= 0xE2
HFUSE ?= 0xD7
EFUSE ?= 0xFD

all: test firmware

test:
	sh test/host/run.sh

assets:
	python3 tools/extract_snake_ex2.py
	python3 tools/convert_assets.py
	python3 tools/gen_assets.py

assets-test:
	python3 tools/test_assets.py

visual:
	python3 tools/test_visual.py

firmware:
	pio run -e ATmega644PA

upload:
	pio run -e ATmega644PA -t upload

fuses:
	$(AVRDUDE) -p m644p -c usbasp -P $(USBASP_PORT) \
	  -U lfuse:w:$(LFUSE):m -U hfuse:w:$(HFUSE):m -U efuse:w:$(EFUSE):m

fuses-read:
	$(AVRDUDE) -p m644p -c usbasp -P $(USBASP_PORT) \
	  -U lfuse:r:-:h -U hfuse:r:-:h -U efuse:r:-:h

# Q5: build/upload a debug image that shows "STACK FREE: NNNN" on the
# pause and game-over screens.  Use `make firmware`/`make upload` afterwards
# to restore the release image.
stack-measure:
	PLATFORMIO_BUILD_FLAGS="-DSTACK_MEASURE=1" pio run -e ATmega644PA

stack-measure-upload:
	PLATFORMIO_BUILD_FLAGS="-DSTACK_MEASURE=1" pio run -e ATmega644PA -t upload

clean:
	rm -rf .pio/build build/assets
