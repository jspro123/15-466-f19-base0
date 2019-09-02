#include "ColorTextureProgram.hpp"

#include "Mode.hpp"
#include "GL.hpp"

#include <vector>
#include <deque>
#define HEX_TO_U8VEC4( HX ) (glm::u8vec4( (HX >> 24) & 0xff, (HX >> 16) & 0xff, (HX >> 8) & 0xff, (HX) & 0xff ))
#define RIGHT 1
#define LEFT 2
#define PLAYER_ONE 1
#define PLAYER_TWO 2
#define MAX_DIST 1.25f
#define OVERLAP_DIST 1.0f
#define ATTACK_SPEED 10.0f
#define ATTACK_COOLDOWN 1.75f
#define REWIND_SPEEDUP 2
#define WALK_SPEED 0.15f
#define MAX_REWIND 1.5f
#define REWIND_COOLDOWN 6.0f

struct player_info {

	glm::u8vec4 normal_colors[5]; //Colors to show normally
	glm::u8vec4 rewind_colors[5]; //Colors to show when rewinding
	glm::u8vec4 cooldown_colors[5]; //Colors to show when cooling down

	glm::vec2 head; //Coordinates
	glm::vec2 head_radius = glm::vec2(0.5f, 0.5f);
	glm::vec4 head_color;

	glm::vec2 torso; //Coordinates
	glm::vec2 torso_radius = glm::vec2(0.20, 1.0f);
	glm::vec4 torso_color;

	glm::vec2 left_arm; //Coordinates
	glm::vec2 left_arm_radius = glm::vec2(-1.25f, 0.4f);
	glm::u8vec4 left_arm_color;
	float left_arm_angle; //The current angle of the left arm

	glm::vec2 right_arm; //Coordinates
	glm::vec2 right_arm_radius = glm::vec2(1.25f, 0.4f);
	glm::u8vec4 right_arm_color;
	float right_arm_angle; //The current angle of the right arm

	glm::vec2 left_leg; //Coordinates
	glm::vec2 left_leg_radius = glm::vec2(0.10f, 1.0f);
	glm::u8vec4 left_leg_color;

	glm::vec2 right_leg; //Coordinates
	glm::vec2 right_leg_radius = glm::vec2(0.10f, 1.0f);
	glm::u8vec4 right_leg_color;

	glm::u8vec4 sword_color;

	//Handles keyboard inputs and state. 0 if false, 1 if true
	int sword_arm = 0; 	//1 if the left player, 2 if the right player
	int left_walk = 0; //1 if holding down the left key, 0 otherwise
	int right_walk = 0; //1 if holding down the right key, 0 otherwise
	int is_attacking = 0; //1 if the sword will kill the other player, 0 otherwise
	int is_rewinding = 0; //1 if rewinding time, 0 otherwise
	int is_cooling; //1 if the player cannot rewind time, 0 otherwise
	float seconds_passed = 0; //Used to limit the amount that the player can rewind
	float seconds_cooldown = 0; //Used to keep track of the cooldown of the move
	std::deque<glm::vec4> rewind_log; //Stores the position of the head, the current angle of the sword arm, and
									  //whether or not the player was attacking

	player_info(glm::vec2 head_coord, int one_or_two, glm::u8vec4* colors, 
				glm::u8vec4* r_colors, glm::u8vec4* c_colors) {

		std::copy(colors, colors + 5, normal_colors);
		std::copy(r_colors, r_colors + 5, rewind_colors);
		std::copy(c_colors, c_colors + 5, cooldown_colors);

		head = head_coord;
		head_color = colors[0];

		torso = glm::vec2(head.x, head.y - 1.50f);
		torso_color = colors[1];

		left_leg = glm::vec2(torso.x - 0.2, torso.y - 2.0f);
		left_leg_color = colors[3];

		right_leg = glm::vec2(torso.x + 0.2, torso.y - 2.0f);
		right_leg_color = colors[3];

		left_arm = glm::vec2(torso.x - 0.2f, torso.y + 0.4f);
		left_arm_color = colors[2];
		left_arm_angle = 60;

		right_arm = glm::vec2(torso.x + 0.2f, torso.y + 0.4f);
		right_arm_color = colors[2];
		right_arm_angle = -60;

		sword_color = colors[4];

		if (one_or_two == PLAYER_ONE) {
			sword_arm = PLAYER_ONE;
		} else if(one_or_two == PLAYER_TWO){
			sword_arm = PLAYER_TWO;
		}

		left_walk = 0;
	    right_walk = 0;
		is_attacking = 0;
		is_rewinding = 0;
		is_cooling = 0;
		seconds_passed = 0; 
		seconds_cooldown = 0;
		rewind_log.clear(); 
	
	}

	void update_coords(glm::vec2 head_coord) {
		head = head_coord;
		torso = glm::vec2(head.x, head.y - 1.50f);
		left_arm = glm::vec2(torso.x - 0.2f, torso.y + 0.4f);
		right_arm = glm::vec2(torso.x + 0.2f, torso.y + 0.4f);
		left_leg = glm::vec2(torso.x - 0.2, torso.y - 2.0f);
		right_leg = glm::vec2(torso.x + 0.2, torso.y - 2.0f);
	}

	void update_colors(int to_update) {

		glm::u8vec4 colors[5];

		if (to_update == 0) { //Normal colors
			std::copy(normal_colors, normal_colors + 5, colors);
		} else if (to_update == 1) { //Rewind colors
			std::copy(rewind_colors, rewind_colors + 5, colors);
		} else if (to_update == 2) { //Cooldown colors
			std::copy(cooldown_colors, cooldown_colors + 5, colors);
		}

		head_color = colors[0];
		torso_color = colors[1];
		left_arm_color = colors[2];
		right_arm_color = colors[2];
		left_leg_color = colors[3];
		right_leg_color = colors[3];
		sword_color = colors[4];
	}

};

struct RewindMode : Mode {
	RewindMode();
	virtual ~RewindMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	glm::vec2 court_radius = glm::vec2(10.0f, 5.0f); 
	uint32_t left_score = 0;
	uint32_t right_score = 0;

	glm::u8vec4 playerOneColors[5] = { HEX_TO_U8VEC4(0x0066ffff), HEX_TO_U8VEC4(0x0066ffff),
									   HEX_TO_U8VEC4(0x0066ffff), HEX_TO_U8VEC4(0x0066ffff),
									   HEX_TO_U8VEC4(0x0066ffff) };

	glm::u8vec4 playerOneRColors[5] = { HEX_TO_U8VEC4(0x006699ff), HEX_TO_U8VEC4(0x006699ff),
									   HEX_TO_U8VEC4(0x006699ff), HEX_TO_U8VEC4(0x006699ff),
									   HEX_TO_U8VEC4(0x006699ff) };

	glm::u8vec4 playerOneCColors[5] = { HEX_TO_U8VEC4(0x00666666), HEX_TO_U8VEC4(0x00666666),
									   HEX_TO_U8VEC4(0x00666666), HEX_TO_U8VEC4(0x00666666),
									   HEX_TO_U8VEC4(0x00666666) };

	glm::u8vec4 playerTwoColors[5] = { HEX_TO_U8VEC4(0xff6600ff), HEX_TO_U8VEC4(0xff6600ff),
									   HEX_TO_U8VEC4(0xff6600ff), HEX_TO_U8VEC4(0xff6600ff),
									   HEX_TO_U8VEC4(0xff6600ff) };

	glm::u8vec4 playerTwoRColors[5] = { HEX_TO_U8VEC4(0x996600ff), HEX_TO_U8VEC4(0x996600ff),
									   HEX_TO_U8VEC4(0x996600ff), HEX_TO_U8VEC4(0x996600ff),
									   HEX_TO_U8VEC4(0x996600ff) };

	glm::u8vec4 playerTwoCColors[5] = { HEX_TO_U8VEC4(0x66660066), HEX_TO_U8VEC4(0x66660066),
									   HEX_TO_U8VEC4(0x66660066), HEX_TO_U8VEC4(0x66660066),
									   HEX_TO_U8VEC4(0x66660066) };

	float sword_tip_length = 2.0f;
	glm::vec2 player_one_init = glm::vec2(-court_radius.x + 2.0f, 0.0f);
	glm::vec2 player_two_init = glm::vec2(court_radius.x - 2.0f, 0.0f);

	player_info playerOne = player_info(player_one_init, 1, playerOneColors, 
										playerOneRColors, playerOneCColors);

	player_info playerTwo = player_info(player_two_init, 2, playerTwoColors, 
										playerTwoRColors, playerTwoCColors);

	//----- opengl assets / helpers ------

	//draw functions will work on vectors of vertices, defined as follows:
	struct Vertex {
		Vertex(glm::vec3 const &Position_, glm::u8vec4 const &Color_, glm::vec2 const &TexCoord_) :
			Position(Position_), Color(Color_), TexCoord(TexCoord_) { }
		glm::vec3 Position;
		glm::u8vec4 Color;
		glm::vec2 TexCoord;
	};
	static_assert(sizeof(Vertex) == 4*3 + 1*4 + 4*2, "PongMode::Vertex should be packed");

	//Shader program that draws transformed, vertices tinted with vertex colors:
	ColorTextureProgram color_texture_program;

	//Buffer used to hold vertex data during drawing:
	GLuint vertex_buffer = 0;

	//Vertex Array Object that maps buffer locations to color_texture_program attribute locations:
	GLuint vertex_buffer_for_color_texture_program = 0;

	//Solid white texture:
	GLuint white_tex = 0;

	//matrix that maps from clip coordinates to court-space coordinates:
	glm::mat3x2 clip_to_court = glm::mat3x2(1.0f);
	// computed in draw() as the inverse of OBJECT_TO_CLIP
	// (stored here so that the mouse handling code can use it to position the paddle)

};
