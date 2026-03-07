BUILD_DIR := build
APPDIR := AppDir
APPIMAGE_TOOL ?= appimagetool
APP_ID := dev.neoapps.LegacyLauncher
VERSION ?= 1.1.0
all: $(BUILD_DIR)
	cmake --build $(BUILD_DIR) --parallel

$(BUILD_DIR):
	cmake -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release

debug:
	cmake -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug
	cmake --build $(BUILD_DIR) --parallel

clean: flatpak-clean
	rm -rf $(BUILD_DIR) $(APPDIR) LegacyLauncher-*.AppImage

install: all
	cmake --install $(BUILD_DIR)

run: all
	./$(BUILD_DIR)/legacy-launcher

appimage: all
	rm -rf $(APPDIR)
	mkdir -p $(APPDIR)/usr/bin
	mkdir -p $(APPDIR)/usr/lib
	mkdir -p $(APPDIR)/usr/share/applications
	mkdir -p $(APPDIR)/usr/share/icons/hicolor/256x256/apps
	mkdir -p $(APPDIR)/usr/plugins/platforms
	mkdir -p $(APPDIR)/usr/plugins/styles
	mkdir -p $(APPDIR)/usr/plugins/imageformats
	mkdir -p $(APPDIR)/usr/plugins/tls
	cp $(BUILD_DIR)/legacy-launcher $(APPDIR)/usr/bin/
	cp packaging/legacy-launcher.desktop $(APPDIR)/usr/share/applications/$(APP_ID).desktop
	cp packaging/legacy-launcher.desktop $(APPDIR)/$(APP_ID).desktop
	cp packaging/icon.png $(APPDIR)/usr/share/icons/hicolor/256x256/apps/$(APP_ID).png
	cp packaging/icon.png $(APPDIR)/$(APP_ID).png
	echo '#!/bin/bash' > $(APPDIR)/AppRun
	echo 'export QT_PLUGIN_PATH="$$APPDIR/usr/plugins:$$QT_PLUGIN_PATH"' >> $(APPDIR)/AppRun
	echo 'export LD_LIBRARY_PATH="$$APPDIR/usr/lib:$$LD_LIBRARY_PATH"' >> $(APPDIR)/AppRun
	echo 'exec "$$APPDIR/usr/bin/legacy-launcher" "$$@"' >> $(APPDIR)/AppRun
	chmod +x $(APPDIR)/AppRun
	@( \
	for lib in $$(ldd $(BUILD_DIR)/legacy-launcher | grep "=>" | awk '{print $$3}'); do \
		[ -f "$$lib" ] && cp -n "$$lib" $(APPDIR)/usr/lib/ 2>/dev/null || true; \
	done; \
	for lib in /usr/lib/x86_64-linux-gnu/libQt6Core.so.6 /usr/lib/x86_64-linux-gnu/libQt6Gui.so.6 /usr/lib/x86_64-linux-gnu/libQt6Widgets.so.6 /usr/lib/x86_64-linux-gnu/libQt6Network.so.6 /usr/lib/x86_64-linux-gnu/libQt6OpenGL.so.6 /usr/lib/x86_64-linux-gnu/libQt6QmlModels.so.6 /usr/lib/x86_64-linux-gnu/libQt6Qml.so.6 /usr/lib/x86_64-linux-gnu/libQt6Quick.so.6; do \
		[ -f "$$lib" ] && cp -n "$$lib" $(APPDIR)/usr/lib/ 2>/dev/null || true; \
	done \
	)

	cp -n /usr/lib64/qt6/plugins/platforms/*.so $(APPDIR)/usr/plugins/platforms/ 2>/dev/null || true
	cp -n /usr/lib64/qt6/plugins/styles/*.so $(APPDIR)/usr/plugins/styles/ 2>/dev/null || true
	cp -n /usr/lib64/qt6/plugins/imageformats/*.so $(APPDIR)/usr/plugins/imageformats/ 2>/dev/null || true
	cp -n /usr/lib64/qt6/plugins/tls/*.so $(APPDIR)/usr/plugins/tls/ 2>/dev/null || true
	cp -n /usr/lib64/qt6/plugins/platforms/*.so $(APPDIR)/usr/plugins/platforms/ 2>/dev/null || true
	cp -n /usr/lib/qt6/plugins/platforms/*.so $(APPDIR)/usr/plugins/platforms/ 2>/dev/null || true
	cp -n /usr/lib/qt6/plugins/styles/*.so $(APPDIR)/usr/plugins/styles/ 2>/dev/null || true
	cp -n /usr/lib/qt6/plugins/imageformats/*.so $(APPDIR)/usr/plugins/imageformats/ 2>/dev/null || true
	cp -n /usr/lib/qt6/plugins/tls/*.so $(APPDIR)/usr/plugins/tls/ 2>/dev/null || true
	cp -n /usr/lib64/libicuuc*.so* $(APPDIR)/usr/lib/ 2>/dev/null || true
	cp -n /usr/lib64/libicudata*.so* $(APPDIR)/usr/lib/ 2>/dev/null || true
	cp -n /usr/lib64/libicui18n*.so* $(APPDIR)/usr/lib/ 2>/dev/null || true
	cp -n /usr/lib64/libdbus-1*.so* $(APPDIR)/usr/lib/ 2>/dev/null || true
	cp -n /usr/lib64/libfontconfig*.so* $(APPDIR)/usr/lib/ 2>/dev/null || true
	cp -n /usr/lib64/libfreetype*.so* $(APPDIR)/usr/lib/ 2>/dev/null || true
	cp -n /usr/lib64/libglib*.so* $(APPDIR)/usr/lib/ 2>/dev/null || true
	cp -n /usr/lib64/libxkbcommon*.so* $(APPDIR)/usr/lib/ 2>/dev/null || true
	cp -n /usr/lib64/libxcb*.so* $(APPDIR)/usr/lib/ 2>/dev/null || true
	cp -n /usr/lib64/libssl*.so* $(APPDIR)/usr/lib/ 2>/dev/null || true
	cp -n /usr/lib64/libcrypto*.so* $(APPDIR)/usr/lib/ 2>/dev/null || true
	cp -n /usr/lib64/libzstd*.so* $(APPDIR)/usr/lib/ 2>/dev/null || true
	cp -n /usr/lib64/libz*.so* $(APPDIR)/usr/lib/ 2>/dev/null || true
	cp -n /usr/lib64/libgmp*.so* $(APPDIR)/usr/lib/ 2>/dev/null || true
	cp -n /usr/lib64/libnss*.so* $(APPDIR)/usr/lib/ 2>/dev/null || true
	cp -n /usr/lib64/libp11-kit*.so* $(APPDIR)/usr/lib/ 2>/dev/null || true
	cp -n /usr/lib64/libidn2*.so* $(APPDIR)/usr/lib/ 2>/dev/null || true
	cp -n /usr/lib64/libunistring*.so* $(APPDIR)/usr/lib/ 2>/dev/null || true
	cp -n /usr/lib64/libk5crypto*.so* $(APPDIR)/usr/lib/ 2>/dev/null || true
	cp -n /usr/lib64/libkrb5*.so* $(APPDIR)/usr/lib/ 2>/dev/null || true
	cp -n /usr/lib64/libsasl2*.so* $(APPDIR)/usr/lib/ 2>/dev/null || true
	cp -n /usr/lib64/libpsl*.so* $(APPDIR)/usr/lib/ 2>/dev/null || true
	cp -n /usr/lib64/liblber*.so* $(APPDIR)/usr/lib/ 2>/dev/null || true
	cp -n /usr/lib64/libldap*.so* $(APPDIR)/usr/lib/ 2>/dev/null || true
	cp -n /usr/lib/libssl*.so* $(APPDIR)/usr/lib/ 2>/dev/null || true
	cp -n /usr/lib/libcrypto*.so* $(APPDIR)/usr/lib/ 2>/dev/null || true
	cp -n /usr/lib/libgmp*.so* $(APPDIR)/usr/lib/ 2>/dev/null || true
	cp -n /usr/lib/libnss*.so* $(APPDIR)/usr/lib/ 2>/dev/null || true
	cp -n /usr/lib/libp11-kit*.so* $(APPDIR)/usr/lib/ 2>/dev/null || true
	@echo -e "[Paths]\nPrefix = .\nPlugins = usr/plugins\nImports = usr/qml\nQml2Imports = usr/qml" > $(APPDIR)/qt.conf
	@cp $(APPDIR)/qt.conf $(APPDIR)/usr/bin/qt.conf
	ARCH=$$(uname -m) $(APPIMAGE_TOOL) $(APPDIR) LegacyLauncher-$(VERSION)-$$(uname -m).AppImage
	@echo "AppImage built: LegacyLauncher-$(VERSION)-$$(uname -m).AppImage"

flatpak: packaging/$(APP_ID).yml
	@command -v flatpak-builder >/dev/null 2>&1 || { echo "flatpak-builder not found."; exit 1; }
	@flatpak remote-list | grep -q flathub || flatpak remote-add --user --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo
	flatpak-builder --user --install-deps-from=flathub --force-clean --repo=flatpak-repo flatpak-build packaging/$(APP_ID).yml
	flatpak build-bundle flatpak-repo LegacyLauncher-$(VERSION).flatpak $(APP_ID)
	@echo "Flatpak bundle built: LegacyLauncher-$(VERSION).flatpak"

flatpak-install: flatpak
	flatpak install --user --bundle LegacyLauncher-$(VERSION).flatpak

flatpak-run:
	flatpak run --user $(APP_ID)

flatpak-clean:
	rm -rf flatpak-build flatpak-repo LegacyLauncher-*.flatpak

.PHONY: all debug clean install run appimage flatpak flatpak-install flatpak-run flatpak-clean
