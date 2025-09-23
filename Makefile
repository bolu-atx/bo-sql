.PHONY: release dev clean test dep

dep:
	pip install meson ninja cmake

release:
	meson setup --buildtype=release build-release
	meson compile -C build-release

dev:
	meson setup --buildtype=debug build-dev
	meson compile -C build-dev

clean:
	rm -rf build-*

test:
	meson test -C build-dev