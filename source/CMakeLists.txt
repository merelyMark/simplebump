CONFIGURE_FILE(
  "${PROJECT_SOURCE_DIR}/src_path.h.in"
  "${PROJECT_SOURCE_DIR}/source/src_path.h"
)

if(WIN32)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DWIN32")
#this adds a preprocessor definition to the project
add_definitions(-DWIN32)
endif()

FIND_PACKAGE(OpenGL)
FIND_PACKAGE(GLU)
INCLUDE_DIRECTORIES(
	"/usr/local/include"
	"/usr/include"
	 ${CMAKE_CURRENT_BINARY_DIR}
	"${PROJECT_SOURCE_DIR}/source"
	"${PROJECT_SOURCE_DIR}/source/freeglut"
	"${PROJECT_SOURCE_DIR}/source/glew/include"
)

message(	 ${CMAKE_CURRENT_BINARY_DIR})
LINK_DIRECTORIES(
	"/usr/lib"
	"/usr/local/lib"
	 ${CMAKE_CURRENT_BINARY_DIR}
	 
)
set(freeglut_SRCS
	freeglut/freeglut_callbacks.c
	freeglut/freeglut_callbacks.c
	freeglut/freeglut_cursor.c
	freeglut/freeglut_display.c
	freeglut/freeglut_ext.c
	freeglut/freeglut_font_data.c
	freeglut/freeglut_font.c
	freeglut/freeglut_gamemode.c
	freeglut/freeglut_geometry.c
	freeglut/freeglut_glutfont_definitions.c
	freeglut/freeglut_init.c
	freeglut/freeglut_input_devices.c
	freeglut/freeglut_internal.h
	freeglut/freeglut_joystick.c
	freeglut/freeglut_main.c
	freeglut/freeglut_menu.c
	freeglut/freeglut_misc.c
	freeglut/freeglut_overlay.c
	freeglut/freeglut_spaceball.c
	freeglut/freeglut_state.c
	freeglut/freeglut_stroke_mono_roman.c
	freeglut/freeglut_stroke_roman.c
	freeglut/freeglut_structure.c
	freeglut/freeglut_teapot_data.h
	freeglut/freeglut_teapot.c
	freeglut/freeglut_videoresize.c
	freeglut/freeglut_window.c
	freeglut/GL/freeglut.h
	freeglut/GL/freeglut_ext.h
	freeglut/GL/freeglut_std.h
	freeglut/GL/glut.h
)

ADD_EXECUTABLE (simplebump 
${freeglut_SRCS}
glew/glew.c
Image/Image.cpp
Maths/COLOR.cpp  
Maths/MATRIX4X4.cpp  
Maths/PLANE.h       
Maths/VECTOR3D.cpp  
Maths/VECTOR4D.h
Maths/COLOR.h    
Maths/MATRIX4X4.h    
Maths/VECTOR2D.cpp  
Maths/VECTOR3D.h
Maths/Maths.h    
Maths/PLANE.cpp      
Maths/VECTOR2D.h    
Maths/VECTOR4D.cpp

NormalisationCubeMap.cpp
TORUS.cpp
main.cpp
)
TARGET_LINK_LIBRARIES(simplebump
${OPENGL_LIBRARIES}
)
