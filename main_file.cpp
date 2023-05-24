/*
Niniejszy program jest wolnym oprogramowaniem; możesz go
rozprowadzać dalej i / lub modyfikować na warunkach Powszechnej
Licencji Publicznej GNU, wydanej przez Fundację Wolnego
Oprogramowania - według wersji 2 tej Licencji lub(według twojego
wyboru) którejś z późniejszych wersji.

Niniejszy program rozpowszechniany jest z nadzieją, iż będzie on
użyteczny - jednak BEZ JAKIEJKOLWIEK GWARANCJI, nawet domyślnej
gwarancji PRZYDATNOŚCI HANDLOWEJ albo PRZYDATNOŚCI DO OKREŚLONYCH
ZASTOSOWAŃ.W celu uzyskania bliższych informacji sięgnij do
Powszechnej Licencji Publicznej GNU.

Z pewnością wraz z niniejszym programem otrzymałeś też egzemplarz
Powszechnej Licencji Publicznej GNU(GNU General Public License);
jeśli nie - napisz do Free Software Foundation, Inc., 59 Temple
Place, Fifth Floor, Boston, MA  02110 - 1301  USA
*/

#define GLM_FORCE_RADIANS
#define GLM_FORCE_SWIZZLE

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include "constants.h"
#include "lodepng.h"
#include "shaderprogram.h"
#include "myCube.h"
#include "myTeapot.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

float speed_x = 0;
float speed_z = 0;
float speed_leg = 0;
float speed_foot = 0;
float aspectRatio = 1;

float* vertexArray;
float* normalArray;
float* uvArray;
int numVerts;

ShaderProgram* sp;

GLuint tex0;
GLuint tex1; //uchwyt na teksturę
GLuint tex2;
GLuint tex3;


//Odkomentuj, żeby rysować kostkę
float* vertices = myCubeVertices;
float* normals = myCubeNormals;
float* texCoords = myCubeTexCoords;
float* colors = myCubeColors;
int vertexCount = myCubeVertexCount;


GLuint readTexture(const char* filename) {
	GLuint tex;
	glActiveTexture(GL_TEXTURE0);
	//Wczytanie do pamięci komputera
	std::vector<unsigned char> image; //Alokuj wektor do wczytania obrazka
	unsigned width, height; //Zmienne do których wczytamy wymiary obrazka
	//Wczytaj obrazek
	unsigned error = lodepng::decode(image, width, height, filename);
	//Import do pamięci karty graficznej
	glGenTextures(1, &tex); //Zainicjuj jeden uchwyt
	glBindTexture(GL_TEXTURE_2D, tex); //Uaktywnij uchwyt
	//Wczytaj obrazek do pamięci KG skojarzonej z uchwytem
	glTexImage2D(GL_TEXTURE_2D, 0, 4, width, height, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, (unsigned char*)image.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	return tex;
}

void load_obj() {
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile("woman.obj", aiProcessPreset_TargetRealtime_Fast);

	if (!scene) {
		std::cout << "Blad wczytywania pliku modelu" << std::endl;
		return;
	}
	else std::cout << "Pobrano model" << std::endl;

	const aiMesh* mesh = scene->mMeshes[0];

	numVerts = mesh->mNumFaces * 3;

	vertexArray = new float[mesh->mNumFaces * 3 * 3];
	normalArray = new float[mesh->mNumFaces * 3 * 3];
	uvArray = new float[mesh->mNumFaces * 3 * 2];

	for (unsigned int i = 0; i < mesh->mNumFaces; i++)
	{
		const aiFace& face = mesh->mFaces[i];

		for (int j = 0; j < 3; j++)
		{
			aiVector3D uv = mesh->mTextureCoords[0][face.mIndices[j]];
			memcpy(uvArray, &uv, sizeof(float) * 2);
			uvArray += 2;

			aiVector3D normal = mesh->mNormals[face.mIndices[j]];
			memcpy(normalArray, &normal, sizeof(float) * 3);
			normalArray += 3;

			aiVector3D pos = mesh->mVertices[face.mIndices[j]];
			memcpy(vertexArray, &pos, sizeof(float) * 3);
			vertexArray += 3;
		}
	}

	uvArray -= mesh->mNumFaces * 3 * 2;
	normalArray -= mesh->mNumFaces * 3 * 3;
	vertexArray -= mesh->mNumFaces * 3 * 3;

}


//Procedura obsługi błędów
void error_callback(int error, const char* description) {
	fputs(description, stderr);
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS) {
		if (key == GLFW_KEY_LEFT) speed_x = -PI / 2;
		if (key == GLFW_KEY_RIGHT) speed_x = PI / 2;
		if (key == GLFW_KEY_UP) {
			speed_z = PI / 2;
			speed_leg = -PI / 4;
			speed_foot = PI / 4;
		}
		if (key == GLFW_KEY_DOWN) {
			speed_z = -PI / 2;
			speed_leg = PI / 4;
			speed_foot = -PI / 4;
		}
	}
	if (action == GLFW_RELEASE) {
		if (key == GLFW_KEY_LEFT) speed_x = 0;
		if (key == GLFW_KEY_RIGHT) speed_x = 0;
		if (key == GLFW_KEY_UP) {
			speed_z = 0;
			speed_foot = 0;
			speed_leg = 0;
		}
		if (key == GLFW_KEY_DOWN) {
			speed_z = 0;
			speed_foot = 0;
			speed_leg = 0;
		}
	}
}

void windowResizeCallback(GLFWwindow* window, int width, int height) {
	if (height == 0) return;
	aspectRatio = (float)width / (float)height;
	glViewport(0, 0, width, height);
}

//Procedura inicjująca
void initOpenGLProgram(GLFWwindow* window) {
	//************Tutaj umieszczaj kod, który należy wykonać raz, na początku programu************
	glClearColor(0.3, 0.3, 0.9, 1);
	glEnable(GL_DEPTH_TEST);
	glfwSetWindowSizeCallback(window, windowResizeCallback);
	glfwSetKeyCallback(window, keyCallback);

	sp = new ShaderProgram("v_simplest.glsl", NULL, "f_simplest.glsl");
	tex0 = readTexture("metal.png");
	tex1 = readTexture("metal_spec.png");
	tex2 = readTexture("grass.png");
	tex3 = readTexture("dirt.png");

	load_obj();
}

//Zwolnienie zasobów zajętych przez program
void freeOpenGLProgram(GLFWwindow* window) {
	//************Tutaj umieszczaj kod, który należy wykonać po zakończeniu pętli głównej************
	delete[] vertexArray;
	delete sp;
}


//Procedura rysująca zawartość sceny
void drawScene(GLFWwindow* window, float angle_x, float z, float angle_leg, float angle_foot) {
	//************Tutaj umieszczaj kod rysujący obraz******************l
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::mat4 V = glm::lookAt(
		glm::vec3(0.0f, 8.0f, -35.0f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f)); //Wylicz macierz widoku

	glm::mat4 P = glm::perspective(50.0f * PI / 180.0f, aspectRatio, 0.01f, 450.0f); //Wylicz macierz rzutowania
	//glm::mat4 P = glm::perspective(glm::radians(50.0f), 1.0f, 1.0f, 50.0f);
	sp->use();//Aktywacja programu cieniującego
	glUniformMatrix4fv(sp->u("P"), 1, false, glm::value_ptr(P));
	glUniformMatrix4fv(sp->u("V"), 1, false, glm::value_ptr(V));


	glm::mat4 floor[10000];
	int k = 0;
	for (float i = -50.0f; i < 50.0f; i += 12.0f) {
		for (float j = -200.0f; j < 20.0f; j += 12.0f) {
			floor[k] = glm::mat4(1.0f);
			floor[k] = glm::translate(floor[k], glm::vec3(i, -10.0f, j));
			floor[k] = glm::scale(floor[k], glm::vec3(6.0f, 0.5f, 6.0f));
			k++;
		}
	}



	//glm::mat4 Floor = glm::mat4(1.0f);
	//Floor = glm::translate(Floor, glm::vec3(0.0f, -10.0f, 0.0f));
	//Floor = glm::scale(Floor, glm::vec3(100.0f, 1.0f, 100.0f));


	glm::mat4 M = glm::mat4(1.0f);
	M = glm::translate(M, glm::vec3(z * std::sin(angle_x), 0.0f, z * std::cos(angle_x))); // Translate along the local Z-axis
	M = glm::rotate(M, angle_x, glm::vec3(0.0f, 1.0f, 0.0f)); // Rotate around the Y-axis

	glm::mat4 Head = M;
	Head = glm::translate(Head, glm::vec3(0.0f, 5.0f, 0.0f));

	glm::mat4 LeftLeg1 = M;
	LeftLeg1 = glm::rotate(LeftLeg1, angle_leg, glm::vec3(1.0f, 0.0f, 0.0f));
	LeftLeg1 = glm::translate(LeftLeg1, glm::vec3(-0.75f, -5.7f, 0.0f));

	glm::mat4 RightLeg1 = M;
	RightLeg1 = glm::rotate(RightLeg1, -angle_leg, glm::vec3(1.0f, 0.0f, 0.0f));
	RightLeg1 = glm::translate(RightLeg1, glm::vec3(0.75f, -5.7f, 0.0f));

	glm::mat4 LeftFoot = LeftLeg1;
	LeftFoot = glm::rotate(LeftFoot, angle_foot, glm::vec3(1.0f, 0.0f, 0.0f));
	LeftFoot = glm::translate(LeftFoot, glm::vec3(0.0f, -2.5f, 0.6f));

	glm::mat4 RightFoot = RightLeg1;
	RightFoot = glm::rotate(RightFoot, -angle_foot, glm::vec3(1.0f, 0.0f, 0.0f));
	RightFoot = glm::translate(RightFoot, glm::vec3(0.0f, -2.5f, 0.6f));

	glm::mat4 LeftArm = M;
	LeftArm = glm::rotate(LeftArm, angle_leg, glm::vec3(-3.0f, -3.0f, 0.0f));
	LeftArm = glm::translate(LeftArm, glm::vec3(2.5f, 1.0f, 0.0f));

	glm::mat4 RightArm = M;
	RightArm = glm::rotate(RightArm, -angle_leg, glm::vec3(-3.0f, 3.0f, 0.0f));
	RightArm = glm::translate(RightArm, glm::vec3(-2.5f, 1.0f, 0.0f));


	M = glm::scale(M, glm::vec3(2.0f, 4.0f, 1.5f));
	//Head = glm::scale(Head, glm::vec3(0.5f, 0.2f, 1.0f));
	LeftLeg1 = glm::scale(LeftLeg1, glm::vec3(0.6f, 2.0f, 1.0f));
	RightLeg1 = glm::scale(RightLeg1, glm::vec3(0.6f, 2.0f, 1.0f));
	LeftFoot = glm::scale(LeftFoot, glm::vec3(0.6f, 0.5f, 1.7f));
	RightFoot = glm::scale(RightFoot, glm::vec3(0.6f, 0.5f, 1.7f));
	LeftArm = glm::scale(LeftArm, glm::vec3(0.6f, 2.0f, 0.7f));
	RightArm = glm::scale(RightArm, glm::vec3(0.6f, 2.0f, 0.7f));

	glm::mat4 array[20] = { M,LeftLeg1,RightLeg1,LeftFoot,RightFoot,LeftArm,RightArm };


	sp->use();//Aktywacja programu cieniującego
	//Przeslij parametry programu cieniującego do karty graficznej
	glUniformMatrix4fv(sp->u("P"), 1, false, glm::value_ptr(P));
	glUniformMatrix4fv(sp->u("V"), 1, false, glm::value_ptr(V));

	for (int i = 0; i <= 6; i++) {
		glUniformMatrix4fv(sp->u("M"), 1, false, glm::value_ptr(array[i]));
		glUniform4f(sp->u("lp"), 0, 0, -6, 1);
		glUniform1i(sp->u("textureMap0"), 0); //drawScene
		glUniform1i(sp->u("textureMap1"), 1);
		glEnableVertexAttribArray(sp->a("vertex"));  //Włącz przesyłanie danych do atrybutu vertex
		glVertexAttribPointer(sp->a("vertex"), 4, GL_FLOAT, false, 0, vertices); //Wskaż tablicę z danymi dla atrybutu vertex
		glEnableVertexAttribArray(sp->a("color"));  //Włącz przesyłanie danych do atrybutu vertex
		glVertexAttribPointer(sp->a("color"), 4, GL_FLOAT, false, 0, colors); //Wskaż tablicę z danymi dla atrybutu vertex
		glEnableVertexAttribArray(sp->a("normal"));  //Włącz przesyłanie danych do atrybutu vertex
		glVertexAttribPointer(sp->a("normal"), 4, GL_FLOAT, false, 0, normals); //Wskaż tablicę z danymi dla atrybutu vertex
		glEnableVertexAttribArray(sp->a("texCoord0"));
		glVertexAttribPointer(sp->a("texCoord0"), 2, GL_FLOAT, false, 0, texCoords);//odpowiednia tablica
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex0);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, tex1);
		glDrawArrays(GL_TRIANGLES, 0, vertexCount); //Narysuj obiekt
	}


	for (int i = 0; i < 10000; i++) {
		glUniformMatrix4fv(sp->u("M"), 1, false, glm::value_ptr(floor[i]));
		glUniform4f(sp->u("lp"), 0, 0, -6, 1);
		glUniform1i(sp->u("textureMap0"), 0); // drawScene
		glUniform1i(sp->u("textureMap1"), 1);

		glEnableVertexAttribArray(sp->a("vertex"));
		glVertexAttribPointer(sp->a("vertex"), 4, GL_FLOAT, false, 0, vertices);

		glEnableVertexAttribArray(sp->a("color"));
		glVertexAttribPointer(sp->a("color"), 4, GL_FLOAT, false, 0, colors);

		glEnableVertexAttribArray(sp->a("normal"));
		glVertexAttribPointer(sp->a("normal"), 4, GL_FLOAT, false, 0, normals);

		glEnableVertexAttribArray(sp->a("texCoord0"));
		glVertexAttribPointer(sp->a("texCoord0"), 2, GL_FLOAT, false, 0, texCoords);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex2);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, tex3);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		glDrawArrays(GL_TRIANGLES, 0, vertexCount);
	}

	//glUniformMatrix4fv(sp->u("M"), 1, false, glm::value_ptr(Head));
	//glUniform4f(sp->u("lp"), 0, 0, -6, 1);
	//glUniform1i(sp->u("textureMap0"), 0); // drawScene
	//glUniform1i(sp->u("textureMap1"), 1);
	//glEnableVertexAttribArray(sp->a("vertex"));
	//glVertexAttribPointer(sp->a("vertex"), 4, GL_FLOAT, false, 0, vertexArray);
	//glEnableVertexAttribArray(sp->a("color"));
	//glVertexAttribPointer(sp->a("color"), 4, GL_FLOAT, false, 0, colors);
	//glEnableVertexAttribArray(sp->a("normal"));
	//glVertexAttribPointer(sp->a("normal"), 4, GL_FLOAT, false, 0, normalArray);
	//glEnableVertexAttribArray(sp->a("texCoord0"));
	//glVertexAttribPointer(sp->a("texCoord0"), 2, GL_FLOAT, false, 0, uvArray);
	//glActiveTexture(GL_TEXTURE0);
	//glBindTexture(GL_TEXTURE_2D, tex0);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	//glActiveTexture(GL_TEXTURE1);
	//glBindTexture(GL_TEXTURE_2D, tex1);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	//glDrawArrays(GL_TRIANGLES, 0, numVerts);


	glDisableVertexAttribArray(sp->a("vertex"));  //Wyłącz przesyłanie danych do atrybutu vertex
	glDisableVertexAttribArray(sp->a("color"));  //Wyłącz przesyłanie danych do atrybutu vertex
	glDisableVertexAttribArray(sp->a("normal"));  //Wyłącz przesyłanie danych do atrybutu vertex
	glDisableVertexAttribArray(sp->a("texCoord0"));


	glfwSwapBuffers(window); //Przerzuć tylny bufor na przedni
}


int main(void)
{
	GLFWwindow* window; //Wskaźnik na obiekt reprezentujący okno

	glfwSetErrorCallback(error_callback);//Zarejestruj procedurę obsługi błędów

	if (!glfwInit()) { //Zainicjuj bibliotekę GLFW
		fprintf(stderr, "Nie można zainicjować GLFW.\n");
		exit(EXIT_FAILURE);
	}

	window = glfwCreateWindow(1080, 1080, "OpenGL", NULL, NULL);  //Utwórz okno 500x500 o tytule "OpenGL" i kontekst OpenGL.

	if (!window) //Jeżeli okna nie udało się utworzyć, to zamknij program
	{
		fprintf(stderr, "Nie można utworzyć okna.\n");
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window); //Od tego momentu kontekst okna staje się aktywny i polecenia OpenGL będą dotyczyć właśnie jego.
	glfwSwapInterval(1); //Czekaj na 1 powrót plamki przed pokazaniem ukrytego bufora

	if (glewInit() != GLEW_OK) { //Zainicjuj bibliotekę GLEW
		fprintf(stderr, "Nie można zainicjować GLEW.\n");
		exit(EXIT_FAILURE);
	}

	initOpenGLProgram(window); //Operacje inicjujące



	//Główna pętla
	float angle_x = 0; //Aktualny kąt obrotu obiektu
	float z = 0; //Aktualny kąt obrotu obiektu
	float angle_leg = 0;
	float angle_foot = 0;
	float x = 1;
	glfwSetTime(0); //Zeruj timer
	while (!glfwWindowShouldClose(window)) //Tak długo jak okno nie powinno zostać zamknięte
	{
		angle_x += speed_x * glfwGetTime(); //Zwiększ/zmniejsz kąt obrotu na podstawie prędkości i czasu jaki upłynał od poprzedniej klatki
		z += speed_z * glfwGetTime(); //Zwiększ/zmniejsz kąt obrotu na podstawie prędkości i czasu jaki upłynał od poprzedniej klatki
		angle_leg += x * speed_leg * glfwGetTime();
		angle_foot += x * speed_foot * glfwGetTime();
		if (angle_leg >= PI / 8 || angle_leg <= -PI / 8) {
			x *= -1;
		}
		glfwSetTime(0); //Zeruj timer
		drawScene(window, angle_x, z, angle_leg, angle_foot); //Wykonaj procedurę rysującą
		glfwPollEvents(); //Wykonaj procedury callback w zalezności od zdarzeń jakie zaszły.
	}

	freeOpenGLProgram(window);

	glfwDestroyWindow(window); //Usuń kontekst OpenGL i okno
	glfwTerminate(); //Zwolnij zasoby zajęte przez GLFW
	exit(EXIT_SUCCESS);
}