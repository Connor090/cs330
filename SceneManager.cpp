///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
// Edited: Connor Holohan 6/7/25
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glDeleteTextures(1, &m_textureIDs[i].ID);  // Changed from glGenTextures this was causing a bug
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}





void SceneManager::SetupSceneLights()
{
	m_pShaderManager->setBoolValue(g_UseLightingName, true);
	m_pShaderManager->setVec3Value("viewPosition", 4.0f, 1.0f, 4.0f);

	

	
	// Point light 1
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.1f, 0.1f, 0.3f);   // blue tint
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", 0.2f, 0.2f, 0.8f);   // deep blue
	m_pShaderManager->setVec3Value("pointLights[0].specular", 0.3f, 0.3f, 1.0f);  // bright blue 
	m_pShaderManager->setVec3Value("pointLights[0].specular", 0.3f, 0.3f, 0.3f);
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);

	// Point light 
	m_pShaderManager->setVec3Value("pointLights[1].position", -77.0f, 10.0f, -27.0f);
	m_pShaderManager->setVec3Value("pointLights[1].ambient", 0.1f, 0.1f, 0.08f);
	m_pShaderManager->setVec3Value("pointLights[1].diffuse", 0.5f, 0.5f, 0.4f);
	m_pShaderManager->setVec3Value("pointLights[1].specular", 0.3, 0.3f, 0.3f);
	m_pShaderManager->setBoolValue("pointLights[1].bActive", true);

	// Disable unused lights
	for (int i = 2; i < 5; i++) {
		m_pShaderManager->setBoolValue("pointLights[" + std::to_string(i) + "].bActive", false);
	}
	m_pShaderManager->setBoolValue("spotLight.bActive", false);
}




void SceneManager::DefineObjectMaterials()
{
	// Floor 
	OBJECT_MATERIAL planeMaterial;
	planeMaterial.tag = "plane";
	planeMaterial.ambientColor = glm::vec3(0.3f, 0.3f, 0.25f);  
	planeMaterial.ambientStrength = 0.3f;                        
	planeMaterial.diffuseColor = glm::vec3(0.8f, 0.8f, 0.8f);   // Light gray
	planeMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f);  
	planeMaterial.shininess = 16.0f;
	m_objectMaterials.push_back(planeMaterial);

	// Cylinder 
	OBJECT_MATERIAL cylinderMaterial;
	cylinderMaterial.tag = "cylinder";
	cylinderMaterial.ambientColor = glm::vec3(0.25f, 0.25f, 0.25f);
	cylinderMaterial.ambientStrength = 0.25f;
	cylinderMaterial.diffuseColor = glm::vec3(0.7f, 0.7f, 0.7f);
	cylinderMaterial.specularColor = glm::vec3(0.3f, 0.3f, 0.3f);
	cylinderMaterial.shininess = 32.0f;
	m_objectMaterials.push_back(cylinderMaterial);

	// Wood dresser material
	OBJECT_MATERIAL boxMaterial;
	boxMaterial.tag = "box";
	boxMaterial.ambientColor = glm::vec3(0.2f, 0.15f, 0.1f);    
	boxMaterial.ambientStrength = 0.4f;                          
	boxMaterial.diffuseColor = glm::vec3(0.6f, 0.4f, 0.2f);     
	boxMaterial.specularColor = glm::vec3(0.4f, 0.4f, 0.4f);
	boxMaterial.shininess = 64.0f;
	m_objectMaterials.push_back(boxMaterial);

	// Brass handles
	OBJECT_MATERIAL sphereMaterial;
	sphereMaterial.tag = "sphere";
	sphereMaterial.ambientColor = glm::vec3(0.3f, 0.25f, 0.1f); 
	sphereMaterial.ambientStrength = 0.2f;                       
	sphereMaterial.diffuseColor = glm::vec3(0.8f, 0.7f, 0.3f);  
	sphereMaterial.specularColor = glm::vec3(0.9f, 0.9f, 0.7f);
	sphereMaterial.shininess = 128.0f;
	m_objectMaterials.push_back(sphereMaterial);

	// Lamp base material (dark metal)
	OBJECT_MATERIAL lampBaseMaterial;
	lampBaseMaterial.tag = "lampBase";
	lampBaseMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	lampBaseMaterial.ambientStrength = 0.3f;
	lampBaseMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	lampBaseMaterial.specularColor = glm::vec3(0.5f, 0.5f, 0.5f);
	lampBaseMaterial.shininess = 64.0f;
	m_objectMaterials.push_back(lampBaseMaterial);

	// Lamp shade material (light fabric)
	OBJECT_MATERIAL lampShadeMaterial;
	lampShadeMaterial.tag = "lampShade";
	lampShadeMaterial.ambientColor = glm::vec3(0.9f, 0.9f, 0.8f);
	lampShadeMaterial.ambientStrength = 0.4f;
	lampShadeMaterial.diffuseColor = glm::vec3(0.95f, 0.95f, 0.9f);
	lampShadeMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f);
	lampShadeMaterial.shininess = 16.0f;
	m_objectMaterials.push_back(lampShadeMaterial);

	// Bed material (blue fabric)
	OBJECT_MATERIAL bedMaterial;
	bedMaterial.tag = "bed";
	bedMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.3f);
	bedMaterial.ambientStrength = 0.3f;
	bedMaterial.diffuseColor = glm::vec3(0.2f, 0.3f, 0.8f);
	bedMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.3f);
	bedMaterial.shininess = 32.0f;
	m_objectMaterials.push_back(bedMaterial);

	// Pillow material (white fabric)
	OBJECT_MATERIAL pillowMaterial;
	pillowMaterial.tag = "pillow";
	pillowMaterial.ambientColor = glm::vec3(0.9f, 0.9f, 0.9f);
	pillowMaterial.ambientStrength = 0.4f;
	pillowMaterial.diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
	pillowMaterial.specularColor = glm::vec3(0.3f, 0.3f, 0.3f);
	pillowMaterial.shininess = 16.0f;
	m_objectMaterials.push_back(pillowMaterial);

	OBJECT_MATERIAL rugMaterial;
	rugMaterial.tag = "rug";
	rugMaterial.ambientColor = glm::vec3(0.3f, 0.1f, 0.1f);    // Dark red tint
	rugMaterial.ambientStrength = 0.4f;
	rugMaterial.diffuseColor = glm::vec3(0.7f, 0.2f, 0.2f);    // Deep red
	rugMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);   // Low shine for fabric
	rugMaterial.shininess = 8.0f;
	m_objectMaterials.push_back(rugMaterial);

}

void SceneManager::LoadSceneTextures()
{
	
		// Load the textures
		CreateGLTexture("textures/oakd.jpg", "oakd");
		CreateGLTexture("textures/oakl.jpg", "oakl");
		CreateGLTexture("textures/brass.jpg", "brass");

		// Bind the loaded textures to texture slots
		BindGLTextures();
	}
/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	
	SetupSceneLights();
	DefineObjectMaterials();

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();    
	m_basicMeshes->LoadSphereMesh();  
	m_basicMeshes->LoadCylinderMesh();  
	m_basicMeshes->LoadConeMesh();      

	CreateGLTexture("textures/oakd.jpg", "oakd");
	CreateGLTexture("textures/oakl.jpg", "oakl");
	CreateGLTexture("textures/brass.jpg", "brass");
	CreateGLTexture("textures/carpet.jpg", "carpet");  
	CreateGLTexture("textures/sheet.jpg", "sheet");   
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()


{
	
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.90f, 0.85f, 0.75f, 1.0f);    //floor plane changing color to match scene


	SetShaderMaterial("plane");
	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	
	/****************************************************************/

	//dresser 
	scaleXYZ = glm::vec3(4.0f, 3.0f, 1.5f);  // Wide and tall dresser
	positionXYZ = glm::vec3(0.0f, 1.5f, -4.0f); // Lift off ground, back a bit
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("oakd"); //dark oak body
	SetShaderMaterial("box");
	m_basicMeshes->DrawBoxMesh();



	// handles
	scaleXYZ = glm::vec3(0.1f); // Small spheres for handles
	SetShaderTexture("brass"); // brass handles
	SetShaderMaterial("sphere");

	int numRows = 3;
	int numCols = 2;
	float startX = -1.2f;
	float startY = 2.5f;
	float spacingX = 2.4f;
	float spacingY = -1.0f;

	for (int row = 0; row < numRows; ++row) {
		for (int col = 0; col < numCols; ++col) {
			float x = startX + col * spacingX;
			float y = startY + row * spacingY;
			float z = -3.2f; // In front of dresser
			positionXYZ = glm::vec3(x, y, z);
			SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
			m_basicMeshes->DrawSphereMesh();

		}
	}
	// using boxes to create drawers
	scaleXYZ = glm::vec3(1.4f, 0.5f, 0.25f); // scaling
	SetShaderTexture("oakl"); //light oak drawers
	SetShaderMaterial("box");
	
	

	for (int row = 0; row < numRows; ++row) {
		for (int col = 0; col < numCols; ++col) {
			float x = startX + col * spacingX;
			float y = startY + row * spacingY;
			float z = -3.25f; // Slightly behind the handle
			positionXYZ = glm::vec3(x, y, z);
			SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
			m_basicMeshes->DrawBoxMesh();
			
		}
	}

	// LAMP - sitting on top of dresser
// Lamp base (box)
	scaleXYZ = glm::vec3(0.4f, 0.2f, 0.4f);  
	positionXYZ = glm::vec3(-1.0f, 3.1f, -4.0f);  
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.2f, 0.2f, 0.2f, 1.0f);  // Dark gray
	SetShaderMaterial("lampBase");
	m_basicMeshes->DrawBoxMesh();

	// Lamp stem (cylinder)
	scaleXYZ = glm::vec3(0.05f, 1.0f, 0.05f);  
	positionXYZ = glm::vec3(-1.0f, 3.2f, -4.0f); 
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.2f, 0.2f, 0.2f, 1.0f);  // dark gray
	SetShaderMaterial("lampBase");
	m_basicMeshes->DrawCylinderMesh();

	// Lamp shade (cone)
	scaleXYZ = glm::vec3(0.6f, 0.5f, 0.6f);  
	positionXYZ = glm::vec3(-1.0f, 3.9f, -4.0f);  
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.95f, 0.95f, 0.9f, 1.0f);  
	SetShaderMaterial("lampShade");
	m_basicMeshes->DrawConeMesh();

	
	

	// Bed 
	scaleXYZ = glm::vec3(4.0f, 1.8f, 7.0f);  
	positionXYZ = glm::vec3(6.0f, 0.4f, -2.0f);  
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("sheet"); 
	SetShaderMaterial("bed");
	m_basicMeshes->DrawBoxMesh();

	// Pillows (two small white boxes)
	scaleXYZ = glm::vec3(1.3f, 0.2f, 0.8f);  // Small pillow 
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);  // White 
	SetShaderMaterial("pillow");

	// Left pillow
	positionXYZ = glm::vec3(5.2f, 1.3f, -4.5f);  // On top of bed left
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	m_basicMeshes->DrawBoxMesh();

	// Right pillow
	positionXYZ = glm::vec3(6.8f, 1.3f, -4.5f);  // On top of bed right
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	m_basicMeshes->DrawBoxMesh();

	// RUG - wide cylinder on the floor
	scaleXYZ = glm::vec3(6.0f, 0.05f, 6.0f);  // Very wide and very flat cylinder
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(2.0f, 0.025f, -1.0f);  // Slightly above floor  centered 
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("carpet");  
	SetShaderMaterial("rug");
	m_basicMeshes->DrawCylinderMesh();

}
