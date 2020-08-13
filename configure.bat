cd external
mkdir install

mkdir glfw_build
cd glfw_build
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DGLFW_BUILD_DOCS=OFF -DGLFW_BUILD_TESTS=OFF -DGLFW_BUILD_EXAMPLES=OFF -DCMAKE_INSTALL_PREFIX="../install/" ../glfw/
cmake --build ./ --config Release
cmake --install ./

cd ..
mkdir string_theory_build
cd string_theory_build
cmake -DCMAKE_BUILD_TYPE=Release -DST_BUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX="../install/" ../string_theory/
cmake --build ./ --config Release
cmake --install ./

cd ..
mkdir libjpeg-turbo_build
cd libjpeg-turbo_build
cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_SHARED=OFF -DENABLE_STATIC=ON -DCMAKE_INSTALL_PREFIX="../install/" ../libjpeg-turbo/
cmake --build ./ --config Release
cmake --install ./

cd ..
mkdir zlib_build
cd zlib_build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="../install/" ../zlib/
cmake --build ./ --config Release
cmake --install ./

cd ..
mkdir libpng_build
cd libpng_build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="../install/share/pkgconfig/;../install/" -DCMAKE_INSTALL_PREFIX="../install/" ../libpng/
cmake --build ./ --config Release
cmake --install ./

cd ..
mkdir libhsplasma_build
cd libhsplasma_build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="../install/share/pkgconfig/;../install/;../install/lib/;../install/lib/libpng/;../install/lib/cmake/string_theory;../install/lib/pkgconfig/;/usr/local/opt/openssl" -DENABLE_PYTHON=OFF -DENABLE_TOOLS=OFF -DENABLE_NET=OFF -DENABLE_PHYSX=OFF -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX="../install/" ../libhsplasma/
cmake --build ./ --config Release
cmake --install ./

cd ../..