# Notes:
# - If getting missing separator error, try replacing spaces with tabs.
# - If using Visual Studio, either run the following commands inside the Visual Studio command prompt (vcvarsall) or remove the Ninja generator from the commands.
.PHONY: build test test_release docs format clean

build:
	make release

release:
	cmake -S ./ -B ./build -G "Ninja Multi-Config" -DENABLE_DEVELOPER_MODE:BOOL=OFF -DCMAKE_BUILD_TYPE:STRING=Release
	cmake --build ./build --config Release

debug:
	cmake -S ./ -B ./build -G "Ninja Multi-Config" -DENABLE_DEVELOPER_MODE:BOOL=ON -DCMAKE_BUILD_TYPE:STRING=Debug
	cmake --build ./build --config Debug

format:
ifeq ($(OS), Windows_NT)
	pwsh -c '$$files=(git ls-files --exclude-standard); foreach ($$file in $$files) { if ((get-item $$file).Extension -in ".cpp", ".hpp", ".c", ".cc", ".cxx", ".hxx", ".ixx") { clang-format -i -style=file $$file } }'
else
	git ls-files --exclude-standard | grep -E '\.(cpp|hpp|c|cc|cxx|hxx|ixx)$$' | xargs clang-format -i -style=file
endif

clean:
ifeq ($(OS), Windows_NT)
	pwsh -c 'function rmrf($$path) { if (test-path $$path) { rm -r -force $$path }}; rmrf ./build;'
else
	rm -rf ./build
endif