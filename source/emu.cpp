
byte ram[4096] = {};

uchar V[16] = {};
ushort I = 0;

ushort stack[16] = {};
uchar sp = 0;

ushort pc = 0;

byte screen[64*32] = {};

bool keys[16] = {};

GLuint display_tex;
GLuint quad_shader;
GLuint quad_vertexbuffer;
GLuint quad_vertexobject;

void emu_init(unsigned char* rom, int rom_size)
{
	(void)rom;

	int num_ops = rom_size / 2;

	Print("num ops: %d", num_ops);

	for (int i = 0 ; i < num_ops ; ++i)
	{
		int op = (rom[2*i] << 8) | rom[2*i+1];

		Printnln("0x2%.2x: %.4x = ", 2*i, op);

		int op_category = op >> 12;
		switch(op_category)
		{
		case 0:
		{
			if (op == 0x00E0)
				Print("CLS");
			else if (op == 0x00EE)
				Print("RET");
			else
				Print("NOP");
			break;
		}
		case 0x1:
		{
			ushort addr = (op & 0xFFF);
			Print("JP 0x%X", addr);
			break;
		}
		case 0x2:
		{
			ushort addr = (op & 0xFFF);
			Print("CALL 0x%X", addr);
			break;
		}
		case 0x3:
		{
			ushort k = (op & 0xFF);
			uchar vx = (op >> 8) & 0xF;
			Print("SE V%X, 0x%X", vx, k);
			break;
		}
		case 0x4:
		{
			ushort k = (op & 0xFF);
			uchar vx = (op >> 8) & 0xF;
			Print("SNE V%X, 0x%X", vx, k);
			break;
		}
		case 0x5:
		{
			Assert((op & 0xF) == 0, "what is even going on");
			uchar vy = (op >> 4) & 0xF;
			uchar vx = (op >> 8) & 0xF;
			Print("SE V%X, V%X", vx, vy);
			break;
		}
		case 0x6:
		{
			ushort literal = (op & 0xFF);
			uchar reg = (op >> 8) & 0xF;
			Print("LD V%X, 0x%X", reg, literal);
			break;
		}
		case 0x7:
		{
			ushort k = (op & 0xFF);
			uchar vx = (op >> 8) & 0xF;
			Print("ADD V%X, 0x%X", vx, k);
			break;
		}
		case 0x8:
		{
			uchar sub_op = (op & 0xF);
			uchar vy = (op >> 4) & 0xF;
			uchar vx = (op >> 8) & 0xF;
			switch (sub_op)
			{
			case 0x0:
				Print("LD V%X, V%X", vx, vy);
				break;
			case 0x1:
				Print("OR V%X, V%X", vx, vy);
				break;
			case 0x2:
				Print("AND V%X, V%X", vx, vy);
				break;
			case 0x3:
				Print("XOR V%X, V%X", vx, vy);
				break;
			case 0x4:
				Print("ADD V%X, V%X", vx, vy);
				break;
			case 0x5:
				Print("SUB V%X, V%X", vx, vy);
				break;
			default:
				Assert(false, "unimplemented op");
				break;
			}
			break;
		}
		case 0x9:
		{
			Assert((op & 0xF) == 0, "what is even going on");
			uchar vy = (op >> 4) & 0xF;
			uchar vx = (op >> 8) & 0xF;
			Print("SNE V%X, V%X", vx, vy);
			break;
		}
		case 0xA:
		{
			ushort literal = (op & 0xFFF);
			Print("LD I, 0x%X", literal);
			break;
		}
		case 0xB:
		{
			ushort addr = (op & 0xFF);
			Print("GP V0, 0x%X", addr);
			break;
		}
		case 0xC:
		{
			ushort n = (op & 0xFF);
			uchar vx = (op >> 8) & 0xF;
			Print("RND V%X, 0x%X", vx, n);
			break;
		}
		case 0xD:
		{
			uchar n = (op & 0xF);
			uchar vy = (op >> 4) & 0xF;
			uchar vx = (op >> 8) & 0xF;
			Print("DRW V%X, V%X, 0x%X", vx, vy, n);
			break;
		}
		case 0xE:
		{	
			ushort subop = op & 0xFF;
			switch (subop)
			{
			case 0xA1:
			{
				uchar vx = (op >> 8) & 0xF;
				Print("SKNP V%X", vx);
				break;
			}
			default:
				Assert(false, "unimplemented command");
				break;
			}
			break;
		}
		case 0xF:
		{
			ushort subop = op & 0xFF;
			uchar vx = (op >> 8) & 0xF;
			switch(subop)
			{
			case 0x07:
				Print("LD V%X, DT", vx);
				break;
			case 0x0A:
				Print("LD V%X, K", vx);
				break;
			case 0x15:
				Print("LD DT, V%X", vx);
				break;
			case 0x18:
				Print("LD ST, V%X", vx);
				break;
			case 0x1E:
				Print("ADD I, V%X", vx);
				break;
			case 0x29:
				Print("LD F, V%X", vx);
				break;
			case 0x33:
				Print("LD B, V%X", vx);
				break;
			case 0x55:
				Print("LD [I], V%X", vx);
				break;
			case 0x65:
				Print("LD V%X, [I]", vx);
				break;
			default:
				Assert(false, "unimplemented op");
				break;
			}
			break;
		}
		default:
			Assert(false, "unimplemented command");
			break;
		}
	}

	// Copy ROM into memory
	for (int i = 0 ; i < rom_size ; ++i)
	{
		ram[0x200+i] = rom[i];
	}
	// Initialize PC to start of ROM location
	pc = 0x200;
	
	
	/* ------------------ Setup opengl resources ------------------ */
	// Create and compile our GLSL program from the shaders
	std::string errors;
	bool success = make_shader_program("shader/quad.vert", "shader/quad.frag",
		quad_shader, errors);
	Assert(success, "failed to compile quad shader: %s", errors.c_str());
	
	glGenTextures(1, &display_tex);
	glBindTexture(GL_TEXTURE_2D, display_tex);
	// Give an empty image to OpenGL ( the last "0" )
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 64, 32, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	// Use box filter
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	
	// Setup vertex buffers and shader for rendering texture to screen
	const GLfloat g_quad_vertex_buffer_data[] = {
		-1.0f, -1.0f,
		3.0f, -1.0f,
		-1.0f,  3.0f,
	};
	glGenBuffers(1, &quad_vertexbuffer);
	glGenVertexArrays(1, &quad_vertexobject);
	glBindVertexArray(quad_vertexobject);
	glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
	GLuint g_AttribLocationPosition = glGetAttribLocation(quad_shader, "Position");
	glEnableVertexAttribArray(g_AttribLocationPosition);
	glVertexAttribPointer(g_AttribLocationPosition, 2, GL_FLOAT, GL_FALSE, 8, 0);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_quad_vertex_buffer_data),
		g_quad_vertex_buffer_data, GL_STATIC_DRAW);
	
	GLuint tex_id = glGetUniformLocation(quad_shader, "inputTex");
	glUseProgram(quad_shader);
	// Set our "inputTex" sampler to user Texture Unit 0
	glUniform1i(tex_id, 0);
	glUseProgram(0);
}

bool emu_sim_step()
{
	int op = (ram[pc] << 8) | ram[pc+1];
	int op_category = op >> 12;
	
	bool drawn = false;
	
	switch(op_category)
	{
	case 0:
	{
		if (op == 0x00E0)
		{
			// CLS
			for (int y = 0 ; y < 32 ; ++y)
			for (int x = 0 ; x < 64 ; ++x)
				screen[64*y + x] = 0;
			pc+=2;
		}
		else if (op == 0x00EE)
		{
			// RET
			Assert(sp > 0, "unmatched return")
			pc = stack[--sp];
			pc+=2;
		}
		else
			Assert(false, "invalid op doofus: 0x%X, pc=%X", op, pc);
		break;
	}
	case 0x1:
	{
		// JP addr
		ushort addr = (op & 0xFFF);
		pc = addr;
		break;
	}
	case 0x2:
	{
		// CALL addr
		ushort addr = (op & 0xFFF);
		Assert(sp < 15, "stack overflow incoming");
		stack[sp] = pc;
		++sp;
		pc = addr;
		break;
	}
	case 0x3:
	{
		// SE Vx, k
		ushort k = (op & 0xFF);
		byte vx = (op >> 8) & 0xF;
		if (V[vx] == k)
			pc+=2;
		pc+=2;
		break;
	}
	case 0x4:
	{
		// SNE Vx, k
		ushort k = (op & 0xFF);
		byte vx = (op >> 8) & 0xF;
		if (V[vx] != k)
			pc+=2;
		pc+=2;
		break;
	}
	case 0x5:
	{
		// SE Vx, Vy
		Assert((op & 0xF) == 0, "what is even going on");
		byte vy = (op >> 4) & 0xF;
		byte vx = (op >> 8) & 0xF;
		if (V[vx] == V[vy])
			pc+=2;
		pc+=2;
		break;
	}
	case 0x6:
	{
		// LD Vx, k
		byte k = (op & 0xFF);
		byte vx = (op >> 8) & 0xF;
		V[vx] = k;
		pc+=2;
		break;
	}
	case 0x7:
	{
		// ADD Vx, k
		byte k = (op & 0xFF);
		byte vx = (op >> 8) & 0xF;
		V[vx] += k;
		pc+=2;
		break;
	}
	case 0x8:
	{
		byte sub_op = (op & 0xF);
		byte vy = (op >> 4) & 0xF;
		byte vx = (op >> 8) & 0xF;
		switch (sub_op)
		{
		case 0x0:
			// LD Vx, Vy
			V[vx] = V[vy];
			break;
		case 0x1:
			// OR Vx, Vy
			V[vx] = V[vx] | V[vy];
			break;
		case 0x2:
			// AND Vx, Vy
			V[vx] = V[vx] & V[vy];
			break;
		case 0x3:
			// XOR Vx, Vy
			V[vx] = V[vx] ^ V[vy];
			break;
		case 0x4:
		{
			// ADD Vx, Vy
			ushort res = V[vx] + V[vy];
			V[0xF] = (res > 0xFF) ? 1 : 0;
			V[vx] = res & 0xFF;
			break;
		}
		case 0x5:
		{
			// SUB Vx, Vy
			short res = V[vx] - V[vy];
			V[0xF] = (res > 0) ? 1 : 0;
			V[vx] = res % 0xFF;
			break;
		}
		default:
			Assert(false, "unimplemented op");
			break;
		}
		pc += 2;
		break;
	}
	case 0x9:
	{
		// SNE Vy, Vy
		Assert((op & 0xF) == 0, "what is even going on");
		byte vy = (op >> 4) & 0xF;
		byte vx = (op >> 8) & 0xF;
		if (V[vx] != V[vy])
			pc+=2;
		pc+=2;
		break;
	}
	case 0xA:
	{
		// LD I, k
		ushort k = (op & 0xFFF);
		I = k;
		pc+=2;
		break;
	}
	case 0xB:
	{
		// JP V0, addr
		byte addr = (op & 0xFF);
		pc = V[0x0] + addr;
		break;
	}
	case 0xC:
	{
		// RND Vx, k
		byte k = (op & 0xFF);
		byte vx = (op >> 8) & 0xF;
		V[vx] = (rand() % 256) & k;
		pc+=2;
		break;
	}
	case 0xD:
	{
		// DRW Vx, Vy, n
		byte n = (op & 0xF);
		byte vy = (op >> 4) & 0xF;
		byte vx = (op >> 8) & 0xF;
		byte carry = 0;
		for (int y = 0 ; y < n ; ++y)
		{
			byte bits = ram[I + y];
			int ty = (V[vy] + y) % 32;
			for (int x = 0 ; x < 8 ; ++x)
			{
				int tx = (V[vx] + x) % 64;
				byte screen_val = screen[64*ty + tx];
				byte sprite_val = (bits >> (7 - x)) & 0x1;
				if (screen_val && sprite_val)
					carry = 1;
				screen[64*ty + tx] = screen_val ^ sprite_val;
			}
		}
		V[0xf] = carry;
		drawn = true;
		pc+=2;
		break;
	}
	case 0xE:
		Assert(false, "unimplemented command");
		break;
	case 0xF:
	{
		ushort subop = op & 0xFF;
		uchar vx = (op >> 8) & 0xF;
		switch(subop)
		{
		case 0x07:
			// LD Vx, DT
			Assert(false, "unimplemented op");
			break;
		case 0x0A:
			// LD Vx, K
			for (byte i = 0 ; i < 0x10 ; ++i)
			{
				if (keys[i])
				{
					V[vx] = i;
					pc+=2;
					break;
				}
			}
			break;
		case 0x18:
			Assert(false, "unimplemented op");
			break;
		case 0x1E:
			// ADD I, Vx
			I = I + V[vx];
			pc+=2;
			break;
		case 0x55:
			// LD [I], Vx
			for (byte v = 0 ; v <= vx ; ++v)
			{
				ram[I + v] = V[v];
			}
			pc+=2;
			break;
		case 0x65:
			// LD Vx, [I]
			for (byte v = 0 ; v <= vx ; ++v)
			{
				V[v] = ram[I + v];
			}
			pc+=2;
			break;
		default:
			Assert(false, "unimplemented op");
			break;
		}
		break;
	}
	default:
		Assert(false, "unimplemented command");
		break;
	}
	
	return drawn;
}

void emu_update()
{
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImColor(IM_COL32_BLACK));
	ImGui::Begin("Display", nullptr,
		ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse);
	{
		ImVec2 canvas_size = ImGui::GetContentRegionAvail();
		canvas_size.x = max(floor(canvas_size.x / 64), 2) * 64;
		canvas_size.y = canvas_size.x / 2;
		ImGui::Image((ImTextureID)display_tex, canvas_size);
	}
	ImGui::End();
	
	ImGuiIO& io = ImGui::GetIO();
	keys[0x1] = io.KeysDown[GLFW_KEY_1];
	keys[0x2] = io.KeysDown[GLFW_KEY_2];
	keys[0x3] = io.KeysDown[GLFW_KEY_3];
	keys[0xC] = io.KeysDown[GLFW_KEY_4];
	keys[0x4] = io.KeysDown[GLFW_KEY_Q];
	keys[0x5] = io.KeysDown[GLFW_KEY_W];
	keys[0x6] = io.KeysDown[GLFW_KEY_E];
	keys[0xD] = io.KeysDown[GLFW_KEY_R];
	keys[0x7] = io.KeysDown[GLFW_KEY_A];
	keys[0x8] = io.KeysDown[GLFW_KEY_S];
	keys[0x9] = io.KeysDown[GLFW_KEY_D];
	keys[0xE] = io.KeysDown[GLFW_KEY_F];
	keys[0xA] = io.KeysDown[GLFW_KEY_Z];
	keys[0x0] = io.KeysDown[GLFW_KEY_X];
	keys[0xB] = io.KeysDown[GLFW_KEY_C];
	keys[0xF] = io.KeysDown[GLFW_KEY_V];
	
	ImGui::Begin("Keys", nullptr,
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);
	{
		static bool key_repeat = false;
		
		ImVec2 canvas_size = ImGui::GetContentRegionAvail();
		#define KEY_BUTTON(key)	{										\
			bool old_key_val = keys[0x##key];							\
			if (old_key_val) 											\
				ImGui::PushStyleColor(ImGuiCol_Button, 					\
					ImColor(IM_COL32_WHITE));							\
			if (!key_repeat) {											\
				if (ImGui::Button(#key)) keys[0x##key] = 1;				\
			} else {													\
				ImGui::Button(#key);									\
				if (ImGui::IsItemActive()) keys[0x##key] = 1;			\
			}															\
			if (old_key_val) ImGui::PopStyleColor();					\
		}
		
		// Line 1
		KEY_BUTTON(1);
		ImGui::SameLine();
		KEY_BUTTON(2);
		ImGui::SameLine();
		KEY_BUTTON(3);
		ImGui::SameLine();
		KEY_BUTTON(C);
		// Line 2
		KEY_BUTTON(4);
		ImGui::SameLine();
		KEY_BUTTON(5);
		ImGui::SameLine();
		KEY_BUTTON(6);
		ImGui::SameLine();
		KEY_BUTTON(D);
		// Line 3
		KEY_BUTTON(7);
		ImGui::SameLine();
		KEY_BUTTON(8);
		ImGui::SameLine();
		KEY_BUTTON(9);
		ImGui::SameLine();
		KEY_BUTTON(E);
		// Line 4
		KEY_BUTTON(A);
		ImGui::SameLine();
		KEY_BUTTON(0);
		ImGui::SameLine();
		KEY_BUTTON(B);
		ImGui::SameLine();
		KEY_BUTTON(F);
		
		ImGui::Checkbox("Repeat", &key_repeat);
	}
	ImGui::End();
	
	ImGui::Begin("Memory", nullptr, 0);
	{
		static int bytes = 1;
		ImGui::Text("Bytes"); ImGui::SameLine();
		bool bytes_updated = ImGui::RadioButton("1", &bytes, 1);
		ImGui::SameLine();
		bytes_updated = bytes_updated || ImGui::RadioButton("2", &bytes, 2);
		ImGui::SameLine();
		bytes_updated = bytes_updated || ImGui::RadioButton("4", &bytes, 4);
		// TODO: when you change bytes per line, scroll needs to readjust
		
		static bool first_update = true;
		
		// TODO: go-to address input - can't input 0 because of strtoul
		static int target_address = 0x200;
		static char address_string[16] = "";
		sprintf(address_string, "0x%.3x", target_address);
		bool text_updated = ImGui::InputText("Address", address_string, 
			ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase | 
			ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_EnterReturnsTrue);
		
		if (text_updated)
		{
			int parsed = strtoul(address_string, null, 16);
			if (parsed > 0)
				target_address = parsed;
		}
		
		bool update_address = text_updated || bytes_updated || first_update;
		
		ImGui::BeginChild("mem", ImGui::GetContentRegionAvail(), false, 0);
		{
			for (int i = 0 ; i < 4096/bytes ; ++i)
			{
				if (bytes == 1)
				{
					ImGui::Text("0x%.3x: 0x%.2x", i, ram[i]);
				}
				else if (bytes == 2)
				{
					int value = (ram[2*i] << 8) | ram[2*i + 1];
					ImGui::Text("0x%.3x: 0x%.4x", i*bytes, value);
				}
				else if (bytes == 4)
				{
					int value = (ram[4*i] << 24) | (ram[4*i+1] << 16) |
						(ram[4*i+2] << 8) | ram[4*i+3];
					ImGui::Text("0x%.3x: 0x%.8x", i*bytes, value);
				}
				
				if (update_address &&
					(i*bytes <= target_address) && (target_address < (i+1)*bytes))
				{
					ImGui::SetScrollHere(0.f);
				}
			}
			
		}
		ImGui::EndChild();
		first_update = false;
	}
	ImGui::End();
	
	// Great for reference
	// ImGui::ShowTestWindow();
	
	// TODO: registers window
	// TODO: execution control window
	// TODO: clock speed control
	// TODO: edit memory - dependency on flow control being implemented
	// TODO: fix issue with key input going to game while trying to use debug tools
	// TODO: Dockable windows
	
	ImGui::PopStyleColor();
}

void emu_render()
{
	glBindTexture(GL_TEXTURE_2D, display_tex);
	
	uint pixels[64*32] = {0};
	for (int i = 0 ; i < 64*32 ; ++i)
	{
		pixels[i] = screen[i] ? (0xff << 24 | 0x00FFFFFF) : 0x00 ;
	}
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE,
		pixels);
	
	glBindTexture(GL_TEXTURE_2D, 0);
}