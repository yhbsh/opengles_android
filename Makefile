TARGETS := list game native triangle video

.PHONY: all clean launch $(TARGETS)

all: $(TARGETS)

list:
	$(MAKE) -C apps/list

game:
	$(MAKE) -C apps/game

native:
	$(MAKE) -C apps/native

triangle:
	$(MAKE) -C apps/triangle

video:
	$(MAKE) -C apps/video

clean:
	$(MAKE) -C apps/list clean
	$(MAKE) -C apps/game clean
	$(MAKE) -C apps/native clean
	$(MAKE) -C apps/triangle clean
	$(MAKE) -C apps/video clean

launch:
	$(MAKE) -C apps/game launch
