OS := $(shell uname)
ARCH := $(shell uname -m)

ifeq ($(OS), Linux)
  ifeq ($(ARCH), x86_64)
    LEAP_LIBRARY := ./LeapDeveloperKit_2.3.1+31549_linux/LeapSDK/lib/x64/libLeap.so -Wl,-rpath,./LeapDeveloperKit_2.3.1+31549_linux/LeapSDK/lib/x64
  else
    LEAP_LIBRARY := ./LeapDeveloperKit_2.3.1+31549_linux/LeapSDK/lib/x86/libLeap.so -Wl,-rpath,./LeapDeveloperKit_2.3.1+31549_linux/LeapSDK/lib/x86
  endif
else
  # OS X
  LEAP_LIBRARY := ./LeapDeveloperKit_2.3.1+31549_linux/LeapSDK/lib/libLeap.dylib
endif

LeapTheremin: LeapTheremin.cpp
	$(CXX) -Wall -g -I./LeapDeveloperKit_2.3.1+31549_linux/LeapSDK/include LeapTheremin.cpp -o LeapTheremin $(LEAP_LIBRARY)
ifeq ($(OS), Darwin)
	install_name_tool -change @loader_path/libLeap.dylib ./LeapDeveloperKit_2.3.1+31549_linux/LeapSDK/lib/libLeap.dylib LeapTheremin
endif

clean:
	rm -rf LeapTheremin LeapTheremin.dSYMS
