cd external
mkdir install

mkdir -p string_theory_build
cd string_theory_build
cmake -DCMAKE_BUILD_TYPE=Release -DST_BUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX="../install/" ../string_theory/
cmake --build ./ --config Release
cmake --install ./

cd ..
mkdir -p libjpeg-turbo_build
cd libjpeg-turbo_build
cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_SHARED=OFF -DENABLE_STATIC=ON -DCMAKE_INSTALL_PREFIX="../install/" ../libjpeg-turbo/
cmake --build ./ --config Release
cmake --install ./

cd ..
mkdir -p libhsplasma_build
cd libhsplasma_build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="../install/lib/cmake/string_theory;../install/lib/pkgconfig/;/usr/local/opt/openssl" -DENABLE_PYTHON=OFF -DENABLE_TOOLS=OFF -DENABLE_NET=OFF -DENABLE_PHYSX=OFF -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX="../install/" ../libhsplasma/
cmake --build ./ --config Release
cmake --install ./

cd ..
mkdir -p glfw_build
cd glfw_build
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DGLFW_BUILD_DOCS=OFF -DGLFW_BUILD_TESTS=OFF -DGLFW_BUILD_EXAMPLES=OFF -DCMAKE_INSTALL_PREFIX="../install/" ../glfw/
cmake --build ./ --config Release
cmake --install ./