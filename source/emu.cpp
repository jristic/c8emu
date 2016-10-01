
byte ram[4096] = {};

uchar V[16] = {};
ushort i = 0;

ushort stack[16] = {};
uchar sp = 0;

ushort pc = 0;

byte screen[64*32] = {};

GLuint quad_shader;
GLuint gl_tex;
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
				Assert(false, "invalid op doofus");
			break;
		}
		case 0x1:
		{
			ushort addr = (op & 0xFFF);
			Print("JP %x", addr);
			break;
		}
		case 0x2:
		{
			ushort addr = (op & 0xFFF);
			Print("CALL %x", addr);
			break;
		}
		case 0x3:
		{
			ushort k = (op & 0xFF);
			uchar vx = (op >> 8) & 0xF;
			Print("SE V%x, %x", vx, k);
			break;
		}
		case 0x4:
		{
			ushort k = (op & 0xFF);
			uchar vx = (op >> 8) & 0xF;
			Print("SNE V%x, %x", vx, k);
			break;
		}
		case 0x5:
		{
			Assert((op & 0xF) == 0, "what is even going on");
			uchar vy = (op >> 4) & 0xF;
			uchar vx = (op >> 8) & 0xF;
			Print("SE V%x, V%x", vx, vy);
			break;
		}
		case 0x6:
		{
			ushort literal = (op & 0xFF);
			uchar reg = (op >> 8) & 0xF;
			Print("LD V%x, %x", reg, literal);
			break;
		}
		case 0x7:
		{
			ushort k = (op & 0xFF);
			uchar vx = (op >> 8) & 0xF;
			Print("ADD V%x, %x", vx, k);
			break;
		}
		case 0x8:
		{
			uchar sub_op = (op & 0xF);
			switch (sub_op)
			{
			case 0x0:
			{
				uchar vy = (op >> 4) & 0xF;
				uchar vx = (op >> 8) & 0xF;
				Print("LD V%x, V%x", vx, vy);
				break;
			}
			case 0x1:
			{
				uchar vy = (op >> 4) & 0xF;
				uchar vx = (op >> 8) & 0xF;
				Print("OR V%x, V%x", vx, vy);
				break;
			}
			case 0x2:
			{
				uchar vy = (op >> 4) & 0xF;
				uchar vx = (op >> 8) & 0xF;
				Print("AND V%x, V%x", vx, vy);
				break;
			}
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
			Print("SNE V%x, V%x", vx, vy);
			break;
		}
		case 0xa:
		{
			ushort literal = (op & 0xFFF);
			Print("LD I, %x", literal);
			break;
		}
		case 0xb:
		{
			ushort addr = (op & 0xFF);
			Print("GP V0, %x", addr);
			break;
		}
		case 0xc:
		{
			ushort n = (op & 0xFF);
			uchar vx = (op >> 8) & 0xF;
			Print("RND V%x, %x", vx, n);
			break;
		}
		case 0xd:
		{
			uchar n = (op & 0xF);
			uchar vy = (op >> 4) & 0xF;
			uchar vx = (op >> 8) & 0xF;
			Print("DRW V%x, V%x, %x", vx, vy, n);
			break;
		}
		case 0xe:
			Assert(false, "unimplemented command");
			break;
		case 0xf:
		{
			ushort subop = op & 0xff;
			switch(subop)
			{
			case 0x0A:
			{
				uchar vx = (op >> 8) & 0xF;
				Print("LD V%x, K", vx);
				break;
			}
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
	
	// Setup opengl resources
	// Create and compile our GLSL program from the shaders
	std::string errors;
	bool success = make_shader_program("shader/quad.vert", "shader/quad.frag", quad_shader, errors);
	Assert(success, "failed to compile quad shader: %s", errors.c_str());
	
	glGenTextures(1, &gl_tex);
	glBindTexture(GL_TEXTURE_2D, gl_tex);
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
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_quad_vertex_buffer_data), g_quad_vertex_buffer_data, GL_STATIC_DRAW);
	
	GLuint tex_id = glGetUniformLocation(quad_shader, "inputTex");
	glUseProgram(quad_shader);
	// Set our "inputTex" sampler to user Texture Unit 0
	glUniform1i(tex_id, 0);
	glUseProgram(0);
}

void emu_update()
{
	int op = (ram[pc] << 8) | ram[pc+1];

	int op_category = op >> 12;
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
			Assert(pc > 0, "unmatched return")
			pc = stack[sp];
			--sp;
			pc+=2;
		}
		else
			Assert(false, "invalid op doofus: %x, pc=%x", op, pc);
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
		switch (sub_op)
		{
		case 0x0:
		{
			// LD Vx, Vy
			byte vy = (op >> 4) & 0xF;
			byte vx = (op >> 8) & 0xF;
			V[vx] = V[vy];
			pc+=2;
			break;
		}
		default:
			Assert(false, "unimplemented op");
			break;
		}
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
	case 0xa:
	{
		// LD I, k
		ushort k = (op & 0xFFF);
		i = k;
		pc+=2;
		break;
	}
	case 0xb:
	{
		// JP V0, addr
		byte addr = (op & 0xFF);
		pc = V[0x0] + addr;
		break;
	}
	case 0xc:
	{
		// RND Vx, k
		byte k = (op & 0xFF);
		byte vx = (op >> 8) & 0xF;
		V[vx] = (rand() % 256) & k;
		pc+=2;
		break;
	}
	case 0xd:
	{
		// DRW Vx, Vy, n
		byte n = (op & 0xF);
		byte vy = (op >> 4) & 0xF;
		byte vx = (op >> 8) & 0xF;
		byte carry = 0;
		for (int y = 0 ; y < n ; ++y)
		{
			byte bits = ram[i + y];
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
		pc+=2;
		break;
	}
	case 0xe:
		Assert(false, "unimplemented command");
		break;
	case 0xf:
		Assert(false, "unimplemented command");
		break;
	default:
		Assert(false, "unimplemented command");
		break;
	}
	
	ImGui::ShowTestWindow();
}

void emu_render()
{
	uint pixels[64*32] = {0};
	for (int i = 0 ; i < 64*32 ; ++i)
	{
		pixels[i] = screen[i] ? (0xff << 24 | 0x00FFFFFF) : 0x00 ;
	}
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	
	// Render texture fullscreen
	glUseProgram(quad_shader);
	// Bind our texture in Texture Unit 0
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gl_tex);
	// Use quad buffer
	glBindVertexArray(quad_vertexobject);
	glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	// Unbind resources
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glUseProgram(0);
}