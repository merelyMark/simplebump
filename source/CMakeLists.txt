CONFIGURE_FILE(
  "${PROJECT_SOURCE_DIR}/src_path.h.in"
  "${PROJECT_SOURCE_DIR}/source/src_path.h"
)

FIND_PACKAGE(OpenGL)
FIND_PACKAGE(GLUT)
FIND_PACKAGE(GLU)
INCLUDE_DIRECTORIES(
	"/usr/local/include"
	"/usr/include"
	 ${CMAKE_CURRENT_BINARY_DIR}
)
LINK_DIRECTORIES(
	"/usr/lib"
	"/usr/local/lib"
	 
)

ADD_EXECUTABLE (simplebump 
Image/IMAGE_CalculateStride.cpp  
Image/IMAGE_LoadBMP.cpp
Image/IMAGE_ExpandPalette.cpp    
Image/IMAGE_LoadCompressedTrueColorTGA.cpp
Image/IMAGE_Load.cpp
Image/IMAGE_Load24BitBMP.cpp     
Image/IMAGE_LoadTGA.cpp
Image/IMAGE_Load8BitBMP.cpp      
Image/IMAGE_LoadUncompressed8BitTGA.cpp
Image/IMAGE_LoadAlpha.cpp        
Image/IMAGE_LoadUncompressedTrueColorTGA.cpp
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
${QT_LIBRARIES}
${OPENGL_LIBRARIES} 
${GLUT_LIBRARIES} 
${GLU_LIBRARIES} 
)
