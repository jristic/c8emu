#define _HAS_EXCEPTIONS 0

#include <string>
#include <imgui.h>
#include "imgui_impl_glfw_gl3.h"
#include <stdio.h>
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include <stdlib.h>
#include <cstring>
#include <time.h>

#include "emu.h"
#include "shader.h"

void Print(const char *str, ...)
{
	char buf[2048];

	va_list ptr;
	va_start(ptr,str);
	vsprintf_s(buf,str,ptr);
	va_end(ptr);

	printf("%s\n", buf);

	OutputDebugString(buf);
	OutputDebugString("\n");
}

void Printnln(const char *str, ...)
{
	char buf[2048];

	va_list ptr;
	va_start(ptr,str);
	vsprintf_s(buf,str,ptr);
	va_end(ptr);

	printf("%s", buf);

	OutputDebugString(buf);
}

void SPrint(char* buf, int buf_size, const char *str, ...)
{
	va_list ptr;
	va_start(ptr,str);
	vsprintf_s(buf,buf_size,str,ptr);
	va_end(ptr);
}

#define null nullptr
typedef unsigned int uint;
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned char byte;

#define Assert(expression, message, ...) 				\
	__pragma(warning(push))								\
	__pragma(warning(suppress:4127))					\
	do { 												\
		if (!(expression)) {							\
			char buf[512];								\
			SPrint(buf, 512,							\
				"/* ---- Assert ---- */ \n"				\
				"LOCATION:  %s@%d		\n"				\
				"CONDITION:  %s			\n"				\
				"MESSAGE: " message "	\n",			\
				__FILE__, __LINE__, 					\
				#expression,							\
				##__VA_ARGS__);							\
			Print("%s", buf);							\
			if (IsDebuggerPresent())					\
			{											\
				DebugBreak();							\
			}											\
			else										\
			{											\
				MessageBoxA(NULL, 						\
					buf,								\
					"Assert Failed", 					\
					MB_ICONERROR | MB_OK);				\
				exit(-1);								\
			}											\
		}												\
	} while (0);										\
	__pragma(warning(pop))								\

static void error_callback(int error, const char* description)
{
	Assert(false, "error_callback: error = %d, desc = %s", error, description);
}

bool should_run = true;

DWORD WINAPI update_thread_main( LPVOID lpParam ) 
{
	(void)lpParam;
	while (should_run)
	{	
		bool drawn = emu_sim_step();
		
		if (drawn)
			Sleep(16);
	}
	
	return 0;
}

int main(int argc, char** argv)
{
	// Setup window
	glfwSetErrorCallback(error_callback);
	if (!glfwInit())
		return 1;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	GLFWwindow* window = glfwCreateWindow(1280, 720, "Chip-8 Emulator", NULL, NULL);
	glfwMakeContextCurrent(window);
	gl3wInit();
	
	// Setup ImGui binding
	ImGui_ImplGlfwGL3_Init(window, true);

	FILE* File;
	uchar rom_buf[4096];
	Assert(argc == 2, "wrong number of args");

	File = fopen(argv[1], "rb");
	Assert(File, "Failed to open file: %s", argv[1]);
	int bytes_read = (int)fread(rom_buf, sizeof(uchar), 4096, File);
	Assert(feof(File), "didn't consume whole file (read=%d): %s", bytes_read, argv[1]);

	Assert(bytes_read % 2 == 0, "odd number of bytes, impossible: %d", bytes_read);

	Print("Read file: %s, bytes: %d", argv[1], bytes_read);

	emu_init(rom_buf, bytes_read);

	srand((int)time(NULL));

	ImVec4 clear_color = ImColor(114, 144, 154);
	
	CreateThread( 
            NULL,                   // default security attributes
            0,                      // use default stack size  
            update_thread_main,     // thread function name
            nullptr,                // argument to thread function 
            0,                      // use default creation flags 
            nullptr);               // returns the thread identifier 

	// Main loop
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
			break;

		ImGui_ImplGlfwGL3_NewFrame();

		emu_update();
		
		// Rendering
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);

		emu_render();

		ImGui::Render();

		glfwSwapBuffers(window);
	}
	
	should_run = false;

	// Cleanup
	ImGui_ImplGlfwGL3_Shutdown();
	glfwTerminate();

	return 0;
}

#include "emu.cpp"
#include "shader.cpp"

#include "imgui.cpp"
#include "imgui_draw.cpp"
#include "imgui_demo.cpp"
#include "imgui_impl_glfw_gl3.cpp"
#include "GL/gl3w.c"
