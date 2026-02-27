# Compiler settings
CXX = g++
CXXFLAGS = -g -Wall -std=gnu++11
INCLUDE_DIRS = \
	-I./support/SDL2/include/SDL2 \
	-I./support/SDL2Image/include/SDL2 \
	-I./support \
	-I./support/curl/include \
	-I./src \
	-I./src/libraries \
	-I./src/engine \
	-I./src/engine/firmware \
	-I./src/widgets \

# Library paths and flags
LIB_DIRS = \
	-L./support/SDL2/lib \
	-L./support/SDL2Image/lib \
	-L./C/msys64/mingw32/lib \
	-L./support
LIBS = \
	-lmingw32 \
	-lSDL2main \
	-lSDL2 \
	-lSDL2_image \
	-mwindows \
	-luser32 \
	-lgdi32 \
	-lwinmm \
	-lsetupapi \
	-luuid \
	-ldxguid \
	-lopengl32 \
	-lglu32 \
	-lcomdlg32 \
	-lhid \
	-lhidapi \
	-lBSL430 \
	-lbass \
	-llibcurl

# Source files
SRCS = \
	./support/GLee.c \
	./src/main.cpp \
	./src/AnalysisAudioView.cpp \
	./src/AnalysisPlots.cpp \
	./src/AnalysisTrainList.cpp \
	./src/AnalysisView.cpp \
	./src/AudioView.cpp \
	./src/CalibrationWindow.cpp \
	./src/ColorDropDownList.cpp \
	./src/ConfigView.cpp \
	./src/DropDownList.cpp \
	./src/FFTView.cpp \
	./src/FirmwareUpdateView.cpp \
	./src/Game.cpp \
	./src/Log.cpp \
	./src/MainView.cpp \
	./src/RecordingBar.cpp \
	./src/ThresholdPanel.cpp \
	./src/defaults/DefaultConfig.cpp \
	./src/engine/AnalysisManager.cpp \
	./src/engine/ArduinoSerial.cpp \
	./src/engine/AudioInputConfig.cpp \
	./src/engine/BASSErrors.cpp \
	./src/engine/EkgBackend.cpp \
	./src/engine/FFTBackend.cpp \
	./src/engine/FileReadUtil.cpp \
	./src/engine/FileRecorder.cpp \
	./src/engine/WavTxtRecorder.cpp \
	./src/engine/FilterBase.cpp \
	./src/engine/HIDUsbManager.cpp \
	./src/engine/HighPassFilter.cpp \
	./src/engine/LowPassFilter.cpp \
	./src/engine/NotchFilter.cpp \
	./src/engine/Player.cpp \
	./src/engine/RecordingManager.cpp \
	./src/engine/SpikeAnalysis.cpp \
	./src/engine/SpikeSorter.cpp \
	./src/engine/firmware/BSLFirmwareUpdater.cpp \
	./src/engine/firmware/BYBFirmwareVO.cpp \
	./src/engine/firmware/FirmwareUpdater.cpp \
	./src/engine/firmware/BYBBootloaderController.cpp \
	./src/libraries/tinyxml2.cpp \
	./src/native/PathsWin.cpp \
	./src/native/SerialPortsScanWin.cpp \
	./src/widgets/Application.cpp \
	./src/widgets/BitmapFontGL.cpp \
	./src/widgets/BoxLayout.cpp \
	./src/widgets/DropDownList.cpp \
	./src/widgets/ErrorBox.cpp \
	./src/widgets/native/FileDialogWin.cpp \
	./src/widgets/HorizontalColorPicker.cpp \
	./src/widgets/HorizontalNumberPicker.cpp \
	./src/widgets/Label.cpp \
	./src/widgets/LayoutItem.cpp \
	./src/widgets/LoadTexture.cpp \
	./src/widgets/Painter.cpp \
	./src/widgets/Plot.cpp \
	./src/widgets/PushButton.cpp \
	./src/widgets/RangeSelector.cpp \
	./src/widgets/ScrollBar.cpp \
	./src/widgets/SwitchLayout.cpp \
	./src/widgets/TabBar.cpp \
	./src/widgets/TextInput.cpp \
	./src/widgets/TextureGL.cpp \
	./src/widgets/ToolTip.cpp \
	./src/widgets/TouchDropDownList.cpp \
	./src/widgets/Widget.cpp \
	./support/CRC.cpp

# Object files
OBJS = $(patsubst %.cpp,%.o,$(filter %.cpp,$(SRCS))) $(patsubst %.c,%.o,$(filter %.c,$(SRCS)))

# Target executable
TARGET = ./win/build/SpikeRecorder.exe

# Build rule
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDE_DIRS) -o $@ $(OBJS) $(LIB_DIRS) $(LIBS) -static-libstdc++ -static-libgcc

# Compile rule for C++ files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDE_DIRS) -c $< -o $@

# Compile rule for C files
%.o: %.c
	$(CXX) $(CXXFLAGS) $(INCLUDE_DIRS) -c $< -o $@

# Clean rule
clean:
	del /Q $(subst /,\,$(OBJS)) $(subst /,\,$(TARGET))

.PHONY: clean