TARGETS := list game audio triangle video

.PHONY: all clean launch $(TARGETS)

all: $(TARGETS)

list:
	$(MAKE) -C apps/list install

game:
	$(MAKE) -C apps/game launch

audio:
	$(MAKE) -C apps/audio install

triangle:
	$(MAKE) -C apps/triangle install

video:
	$(MAKE) -C apps/video install

clean:
	$(MAKE) -C apps/list clean
	$(MAKE) -C apps/game clean
	$(MAKE) -C apps/audio clean
	$(MAKE) -C apps/triangle clean
	$(MAKE) -C apps/video clean

launch:
	$(MAKE) -C apps/game launch
