# PRPViewer

This is a realtime viewer for Uru and Myst V game assets. It relies on the open-sourced Plasma engine to load levels and extract enough information to perform realtime rendering. This project depends on glfw and libhsplasma via Cmake, and was only tested on macOS. It should build on Windows and Linux with a bit of manual intervention in the build files. You will also need most of the dependencies from the Plasma engine, as listed [here](https://github.com/H-uru/libhsplasma)

More information in this series of blog posts:

* [Coming back to Uru, part 1](http://blog.simonrodriguez.fr/articles/10-04-2018_coming_back_to_uru.html)  
* [Coming back to Uru, part 2](http://blog.simonrodriguez.fr/articles/22-04-2018_coming_back_to_uru_part_2.html)  
* [Coming back to Uru, part 3](http://blog.simonrodriguez.fr/articles/25-10-2018_coming_back_to_uru_part_3.html )  
* [Coming back to Uru, part 4](http://blog.simonrodriguez.fr/articles/02-11-2018_coming_back_to_uru_part_4.html)

![Result 1](images/result0.jpg)
![Result 2](images/result1.jpg)
![Result 3](images/result2.jpg)
![Result 4](images/result3.jpg)

# License
Libraries in `src/libs/*` and `external/*` remain under their respective licenses.
This work is based on the awesome [libhsplasma](https://github.com/H-uru/libhsplasma) project, licensed under GPLv3. PRPViewer is thus also licensed under the [GPLv3](https://github.com/kosua20/PRPViewer/blob/master/LICENSE).
