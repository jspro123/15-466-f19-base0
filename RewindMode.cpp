#include "RewindMode.hpp"

//for the GL_ERRORS() macro:
#include "gl_errors.hpp"

//for glm::value_ptr() :
#include <glm/gtc/type_ptr.hpp>

#include <random>

//Returns sin(angle) in degrees.
double get_sin(float angle) 
{
	double pi = 3.14159265;
	return sin(angle * pi / 180);
}

//Returns cos(angle) in degrees.
double get_cos(float angle)
{
	double pi = 3.14159265;
	return cos(angle * pi / 180);
}

//Calculates the vertices of the sword of the given player.
//Note that the sword is basically a triangle.
void get_sword_points(player_info& player, float sword_tip_length, glm::vec2 points[]) {
	double sin_angle;
	double cos_angle;
	glm::vec2 pos_1;
	glm::vec2 pos_2;
	glm::vec2 pos_3;

	if (player.sword_arm == PLAYER_ONE) { //Left player

		sin_angle = get_sin(player.right_arm_angle);
		cos_angle = get_cos(player.right_arm_angle);

		pos_1 = glm::vec2(player.right_arm.x + player.right_arm_radius.x * cos_angle,
						  player.right_arm.y - player.right_arm_radius.x * sin_angle);
		pos_2 = glm::vec2(pos_1.x + sword_tip_length * cos_angle, pos_1.y - sword_tip_length * sin_angle);
		pos_3 = glm::vec2(player.right_arm.x + player.right_arm_radius.x * cos_angle,
						  player.right_arm.y - player.right_arm_radius.x * sin_angle - player.right_arm_radius.y);
	}
	else if (player.sword_arm == PLAYER_TWO) { //Right player

		sin_angle = get_sin(player.left_arm_angle);
		cos_angle = get_cos(player.left_arm_angle);

		pos_1 = glm::vec2(player.left_arm.x + player.left_arm_radius.x * cos_angle,
						  player.left_arm.y - player.left_arm_radius.x * sin_angle);
		pos_2 = glm::vec2(pos_1.x - sword_tip_length * cos_angle, pos_1.y + sword_tip_length * sin_angle);
		pos_3 = glm::vec2(player.left_arm.x + player.left_arm_radius.x * cos_angle,
						  player.left_arm.y - player.left_arm_radius.x * sin_angle - player.left_arm_radius.y);
	}

	points[0] = pos_1;
	points[1] = pos_2;
	points[2] = pos_3;
}

//When a player is rewinding time, this function checks whether
//time's up (lol) and determines their new position/state.
void update_rewind(player_info& player, float elapsed) {

	int skipped = 0;

	//The way I boost the speed of the player while they're rewinding time is by
	//dropping past coordinates and moving on to the next; that's exactly what 
	//I do in this loop
	while (skipped < REWIND_SPEEDUP) {
		if (!player.rewind_log.empty() && player.seconds_passed < MAX_REWIND) {
			player.rewind_log.pop_front();
		} else {
			player.is_rewinding = 0;
			player.is_cooling = 1;
			player.seconds_passed = 0;
			player.update_colors(2);
			return;
		}
		skipped += 1;
	}

	if (!player.rewind_log.empty()) {
			glm::vec4 past_info = player.rewind_log.front();
			player.update_coords(glm::vec2(past_info.x, past_info.y));
			if (player.sword_arm == PLAYER_ONE) {
				player.right_arm_angle = past_info.z;
			} else {
				player.left_arm_angle = past_info.z;
			}
			player.is_attacking = (int)past_info.w;
			player.seconds_passed += elapsed;
			player.rewind_log.pop_front();
	} else {
			player.is_rewinding = 0;
			player.is_cooling = 1;
			player.seconds_passed = 0;
			player.update_colors(2);
	}
}

//Determines the player's (not the other player!) new position, and ensures 
//that the two players don't collide with each other - there's an invisble
//wall seperating them. Also makes sure that the players can't go through the
//walls of the arena.
void update_movements(int right_or_left, int one_or_two, player_info& player,
					  player_info& other_player, glm::vec2 court_radius, float sword_tip_length) {
	if (one_or_two == PLAYER_ONE) {
		if (right_or_left == RIGHT) {

			glm::vec2 points[3];
			get_sword_points(player, sword_tip_length, points);
			glm::vec2 pos = points[1];

			if (pos.x <= court_radius.x && pos.x >= -court_radius.x &&
				!(abs(player.head.x - other_player.head.x) <= MAX_DIST)) {
				player.update_coords(glm::vec2(player.head.x + WALK_SPEED, 0.0f));
			}
		}

		else if (right_or_left == LEFT) {

			double sin_angle = get_sin(player.left_arm_angle);
			double cos_angle = get_cos(player.left_arm_angle);
			glm::vec2 pos = glm::vec2(player.left_arm.x + player.left_arm_radius.x * cos_angle,
									  player.left_arm.y - player.left_arm_radius.x * sin_angle);

			if (pos.x <= court_radius.x && pos.x >= -court_radius.x) {
				player.update_coords(glm::vec2(player.head.x - WALK_SPEED, 0.0f));
			}
		}
	} else if (one_or_two == PLAYER_TWO){
		if (right_or_left == RIGHT) {

			double sin_angle = get_sin(player.right_arm_angle);
			double cos_angle = get_cos(player.right_arm_angle);

			glm::vec2 pos = glm::vec2(player.right_arm.x + player.right_arm_radius.x * cos_angle,
									  player.right_arm.y - player.right_arm_radius.x * sin_angle);

			if (pos.x <= court_radius.x && pos.x >= -court_radius.x) {
				player.update_coords(glm::vec2(player.head.x + WALK_SPEED, 0.0f));
			}
		}

		else if (right_or_left == LEFT) {

			glm::vec2 points[3];
			get_sword_points(player, sword_tip_length, points);
			glm::vec2 pos = points[1];

			if (pos.x <= court_radius.x && pos.x >= -court_radius.x &&
				!(abs(player.head.x - other_player.head.x) <= MAX_DIST)) {
				player.update_coords(glm::vec2(player.head.x - WALK_SPEED, 0.0f));
			}
		}
	}
}

RewindMode::RewindMode() {

	//----- allocate OpenGL resources -----
	{ //vertex buffer:
		glGenBuffers(1, &vertex_buffer);
		//for now, buffer will be un-filled.

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}

	{ //vertex array mapping buffer for color_texture_program:
		//ask OpenGL to fill vertex_buffer_for_color_texture_program with the name of an unused vertex array object:
		glGenVertexArrays(1, &vertex_buffer_for_color_texture_program);

		//set vertex_buffer_for_color_texture_program as the current vertex array object:
		glBindVertexArray(vertex_buffer_for_color_texture_program);

		//set vertex_buffer as the source of glVertexAttribPointer() commands:
		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

		//set up the vertex array object to describe arrays of PongMode::Vertex:
		glVertexAttribPointer(
			color_texture_program.Position_vec4, //attribute
			3, //size
			GL_FLOAT, //type
			GL_FALSE, //normalized
			sizeof(Vertex), //stride
			(GLbyte*)0 + 0 //offset
		);
		glEnableVertexAttribArray(color_texture_program.Position_vec4);
		//[Note that it is okay to bind a vec3 input to a vec4 attribute -- the w component will be filled with 1.0 automatically]

		glVertexAttribPointer(
			color_texture_program.Color_vec4, //attribute
			4, //size
			GL_UNSIGNED_BYTE, //type
			GL_TRUE, //normalized
			sizeof(Vertex), //stride
			(GLbyte*)0 + 4 * 3 //offset
		);
		glEnableVertexAttribArray(color_texture_program.Color_vec4);

		glVertexAttribPointer(
			color_texture_program.TexCoord_vec2, //attribute
			2, //size
			GL_FLOAT, //type
			GL_FALSE, //normalized
			sizeof(Vertex), //stride
			(GLbyte*)0 + 4 * 3 + 4 * 1 //offset
		);
		glEnableVertexAttribArray(color_texture_program.TexCoord_vec2);

		//done referring to vertex_buffer, so unbind it:
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		//done setting up vertex array object, so unbind it:
		glBindVertexArray(0);

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}

	{ //solid white texture:
		//ask OpenGL to fill white_tex with the name of an unused texture object:
		glGenTextures(1, &white_tex);

		//bind that texture object as a GL_TEXTURE_2D-type texture:
		glBindTexture(GL_TEXTURE_2D, white_tex);

		//upload a 1x1 image of solid white to the texture:
		glm::uvec2 size = glm::uvec2(1, 1);
		std::vector< glm::u8vec4 > data(size.x * size.y, glm::u8vec4(0xff, 0xff, 0xff, 0xff));
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());

		//set filtering and wrapping parameters:
		//(it's a bit silly to mipmap a 1x1 texture, but I'm doing it because you may want to use this code to load different sizes of texture)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		//since texture uses a mipmap and we haven't uploaded one, instruct opengl to make one for us:
		glGenerateMipmap(GL_TEXTURE_2D);

		//Okay, texture uploaded, can unbind it:
		glBindTexture(GL_TEXTURE_2D, 0);

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}
}

RewindMode::~RewindMode() {

	//----- free OpenGL resources -----
	glDeleteBuffers(1, &vertex_buffer);
	vertex_buffer = 0;

	glDeleteVertexArrays(1, &vertex_buffer_for_color_texture_program);
	vertex_buffer_for_color_texture_program = 0;

	glDeleteTextures(1, &white_tex);
	white_tex = 0;
}

bool RewindMode::handle_event(SDL_Event const& evt, glm::uvec2 const& window_size) {

	if (evt.type == SDL_MOUSEMOTION) {
		//convert mouse from window pixels (top-left origin, +y is down) to clip space ([-1,1]x[-1,1], +y is up):
		glm::vec2 clip_mouse = glm::vec2(
			(evt.motion.x + 0.5f) / window_size.x * 2.0f - 1.0f,
			(evt.motion.y + 0.5f) / window_size.y * -2.0f + 1.0f
		);
	} else if (evt.type == SDL_KEYDOWN) {
		// Left Player stuff
		if (evt.key.keysym.sym == SDLK_d) { //Walk right

			playerOne.right_walk = 1;
		}

		if (evt.key.keysym.sym == SDLK_a) { //Walk left

			playerOne.left_walk = 1;
		}

		if (evt.key.keysym.sym == SDLK_w && playerOne.right_arm_angle == -60) { //Attack
			playerOne.is_attacking = 1;
		}

		if (evt.key.keysym.sym == SDLK_s && playerOne.is_cooling == 0 && playerTwo.is_rewinding == 0) { //Rewind
			playerOne.update_colors(1);
			playerOne.is_rewinding = 1;
		}

		//Right player stuff
		if (evt.key.keysym.sym == SDLK_LEFT) { //Walk left

			playerTwo.left_walk = 1;
		}

		if (evt.key.keysym.sym == SDLK_RIGHT) { //Walk right

			playerTwo.right_walk = 1;
		}

		if (evt.key.keysym.sym == SDLK_UP && playerTwo.left_arm_angle == 60) { //Attack
			playerTwo.is_attacking = 1;
		}

		if (evt.key.keysym.sym == SDLK_DOWN && playerTwo.is_cooling == 0 && playerOne.is_rewinding == 0) { //Rewind
			playerTwo.update_colors(1);
			playerTwo.is_rewinding = 1;
		}

	} else if (evt.type == SDL_KEYUP) {

		//Left Player

		if (evt.key.keysym.sym == SDLK_d) {

			playerOne.right_walk = 0;
		}

		if (evt.key.keysym.sym == SDLK_a) {

			playerOne.left_walk = 0;
		}

		if (evt.key.keysym.sym == SDLK_s && playerOne.is_rewinding == 1) {
			playerOne.is_rewinding = 0;
			playerOne.is_cooling = 1;
			playerOne.seconds_passed = 0;
			playerOne.update_colors(2);
		}

		// Right player
		if (evt.key.keysym.sym == SDLK_LEFT) {

			playerTwo.left_walk = 0;
		}

		if (evt.key.keysym.sym == SDLK_RIGHT) {

			playerTwo.right_walk = 0;
		}

		if (evt.key.keysym.sym == SDLK_DOWN && playerTwo.is_rewinding == 1) {
			playerTwo.is_rewinding = 0;
			playerTwo.is_cooling = 1;
			playerTwo.seconds_passed = 0;
			playerTwo.update_colors(2);
		}

	}

	return false;
}

void RewindMode::update(float elapsed) {

	//It is not possible for both players to rewind at the same time
	assert((playerOne.is_rewinding == 1 && playerTwo.is_rewinding == 0) ||
			(playerOne.is_rewinding == 0 && playerTwo.is_rewinding == 1) ||
			(playerOne.is_rewinding == 0 && playerTwo.is_rewinding == 0));

	/* --------------- MOVEMENT AND ATTACKS  --------------- */

	//For the player on the left
	if (playerOne.is_rewinding == 0) {
		if (playerOne.is_cooling == 1) {
			if (playerOne.seconds_cooldown >= REWIND_COOLDOWN) {
				playerOne.update_colors(0);
				playerOne.is_cooling = 0;
				playerOne.seconds_cooldown = 0;
			}
			playerOne.seconds_cooldown += elapsed;
		}

		if (playerOne.is_attacking == 1 && playerOne.right_arm_angle <= 0) {
			playerOne.right_arm_angle += elapsed + ATTACK_SPEED;
			if (playerOne.right_arm_angle >= 0) {
				playerOne.is_attacking = 0;
				playerOne.right_arm_angle = 0;
			}
		}
		else if (playerOne.is_attacking == 0 && playerOne.right_arm_angle > -60) {
			playerOne.right_arm_angle += elapsed - ATTACK_COOLDOWN;
			if (playerOne.right_arm_angle <= -60) {
				playerOne.right_arm_angle = -60;
			}
		}

		//If you're holding both keys down, you don't move
		if (playerOne.left_walk == 1 && playerOne.right_walk == 0) {
			update_movements(LEFT, PLAYER_ONE, playerOne, playerTwo,
							court_radius, sword_tip_length);
		}
		else if (playerOne.left_walk == 0 && playerOne.right_walk == 1) {
			update_movements(RIGHT, PLAYER_ONE, playerOne, playerTwo,
							court_radius, sword_tip_length);
		}

		//For later if the player decides to rewind time
		playerOne.rewind_log.push_front(glm::vec4(playerOne.head.x, playerOne.head.y, 
								playerOne.right_arm_angle, playerOne.is_attacking));

	} else if (playerOne.is_rewinding == 1) {
		update_rewind(playerOne, elapsed);
	}

	//For the player on the right
	if (playerTwo.is_rewinding == 0) {
		if (playerTwo.is_cooling == 1) {
			if (playerTwo.seconds_cooldown >= REWIND_COOLDOWN) {
				playerTwo.update_colors(0);
				playerTwo.is_cooling = 0;
				playerTwo.seconds_cooldown = 0;
			}
			playerTwo.seconds_cooldown += elapsed;
		}

		if (playerTwo.is_attacking == 1 && playerTwo.left_arm_angle <= 60) {
			playerTwo.left_arm_angle -= elapsed + ATTACK_SPEED;
			if (playerTwo.left_arm_angle <= 0) {
				playerTwo.is_attacking = 0;
				playerTwo.left_arm_angle = 0;
			}
		}
		else if (playerTwo.is_attacking == 0 && playerTwo.left_arm_angle < 60) {
			playerTwo.left_arm_angle += elapsed + ATTACK_COOLDOWN;
			if (playerTwo.left_arm_angle >= 60) {
				playerTwo.left_arm_angle = 60;
			}
		}

		if (playerTwo.left_walk == 1 && playerTwo.right_walk == 0) {
			update_movements(LEFT, PLAYER_TWO, playerTwo, playerOne,
				court_radius, sword_tip_length);
		}
		else if (playerTwo.left_walk == 0 && playerTwo.right_walk == 1) {
			update_movements(RIGHT, PLAYER_TWO, playerTwo, playerOne,
				court_radius, sword_tip_length);
		}

		playerTwo.rewind_log.push_front(glm::vec4(playerTwo.head.x, playerTwo.head.y,
			playerTwo.left_arm_angle, playerTwo.is_attacking));

	} else if (playerTwo.is_rewinding == 1) {
		update_rewind(playerTwo, elapsed);
	}

	/* ---------------- COLLISION DETECTION ---------------- */

	//Check for temporal collisions in case someone is rewinding:
	//In case of a collision, the rewinding player loses. 
	if (playerOne.is_rewinding == 1) {
		if (abs(playerOne.head.x - playerTwo.head.x) <= OVERLAP_DIST) {
			right_score += 1;
			playerOne = player_info(player_one_init, 1, playerOneColors,
									playerOneRColors, playerOneCColors);

			playerTwo = player_info(player_two_init, 2, playerTwoColors,
									playerTwoRColors, playerTwoCColors);
			return;
		}
	} else if (playerTwo.is_rewinding == 1) {
		if (abs(playerOne.head.x - playerTwo.head.x) <= OVERLAP_DIST) {
			left_score += 1;
			playerOne = player_info(player_one_init, 1, playerOneColors,
									playerOneRColors, playerOneCColors);

			playerTwo = player_info(player_two_init, 2, playerTwoColors,
									playerTwoRColors, playerTwoCColors);
			return;
		}
	}

	/* -------------------- HIT DETECTION -------------------- */

	int player_one_wins = 0;
	int player_two_wins = 0;

	if (playerOne.is_attacking == 1) {
		glm::vec2 points[3];
		get_sword_points(playerOne, sword_tip_length, points);
		
		//Check if the sword hit the head
		if (points[1].x >= playerTwo.head.x - playerTwo.head_radius.x &&
			points[1].y <= playerTwo.head.y + playerTwo.head_radius.y){
			player_one_wins = 1;
		}
		//Check if the sword hit the torso
		if (points[1].x >= playerTwo.torso.x - playerTwo.torso_radius.x &&
			points[1].y <= playerTwo.torso.y + playerTwo.torso_radius.y) {
			player_one_wins = 1;
		}
	}

	if (playerTwo.is_attacking == 1) {
		glm::vec2 points[3];
		get_sword_points(playerTwo, sword_tip_length, points);

		//Check if the sword hit the head
		if (points[1].x <= playerOne.head.x + playerOne.head_radius.x &&
			points[1].y <= playerOne.head.y + playerOne.head_radius.y) {
			player_two_wins = 1;
		}
		//Check if the sword hit the torso
		if (points[1].x <= playerOne.torso.x + playerOne.torso_radius.x &&
			points[1].y <= playerOne.torso.y + playerOne.torso_radius.y) {
			player_two_wins = 1;
		}
	}

	/* -------------------- END ROUND -------------------- */

	if (player_one_wins == 0 && player_two_wins == 0) {
		return;
	}

	if (player_one_wins == 1) {
		left_score += 1;
	} 
	if (player_two_wins == 1) {
		right_score += 1;
	}

	playerOne = player_info(player_one_init, 1, playerOneColors,
							playerOneRColors, playerOneCColors);

	playerTwo = player_info(player_two_init, 2, playerTwoColors,
							playerTwoRColors, playerTwoCColors);

}

void RewindMode::draw(glm::uvec2 const& drawable_size) {
	//some nice colors from the course web page:
	const glm::u8vec4 bg_color = HEX_TO_U8VEC4(0xf3ffc6ff);
	const glm::u8vec4 fg_color = HEX_TO_U8VEC4(0x000000ff);

	//other useful drawing constants:
	const float wall_radius = 0.05f;
	const float padding = 0.05f; //padding between outside of walls and edge of window

	//vertices will be accumulated into this list and then uploaded+drawn at the end of this function:
	std::vector< Vertex > vertices;

	//inline helper function for rectangle drawing:
	auto draw_rectangle = [&vertices](glm::vec2 const& center, glm::vec2 const& radius, glm::u8vec4 const& color) {
		//split rectangle into two CCW-oriented triangles:
		vertices.emplace_back(glm::vec3(center.x - radius.x, center.y - radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x + radius.x, center.y - radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x + radius.x, center.y + radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));

		vertices.emplace_back(glm::vec3(center.x - radius.x, center.y - radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x + radius.x, center.y + radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x - radius.x, center.y + radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
	};

	//For angling those pesky rectangles (given two fixed points).
	//Used when drawing the arms.
	auto draw_angle_rectangle = [&vertices](glm::vec2 const& arm, glm::vec2 const arm_radius, glm::u8vec4 const& color, float angle) {

		double sin_angle = get_sin(angle);
		double cos_angle = get_cos(angle);

		glm::vec2 pos_1 = glm::vec2(arm.x, arm.y);
		glm::vec2 pos_2 = glm::vec2(arm.x, arm.y - arm_radius.y);
		glm::vec2 pos_3 = glm::vec2(arm.x + arm_radius.x * cos_angle, arm.y - arm_radius.x * sin_angle);
		glm::vec2 pos_4 = glm::vec2(arm.x + arm_radius.x * cos_angle, arm.y - arm_radius.x * sin_angle - arm_radius.y);

		vertices.emplace_back(glm::vec3(pos_1.x, pos_1.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(pos_2.x, pos_2.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(pos_3.x, pos_3.y, 0.0f), color, glm::vec2(0.5f, 0.5f));

		vertices.emplace_back(glm::vec3(pos_3.x, pos_3.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(pos_2.x, pos_2.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(pos_4.x, pos_4.y, 0.0f), color, glm::vec2(0.5f, 0.5f));

	};

	auto draw_sword = [&vertices](player_info& player, float sword_tip_length) {

		glm::vec2 points[3];
		get_sword_points(player, sword_tip_length, points);
		glm::vec2 pos_1 = points[0];
		glm::vec2 pos_2 = points[1];
		glm::vec2 pos_3 = points[2];
		vertices.emplace_back(glm::vec3(pos_1.x, pos_1.y, 0.0f), player.sword_color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(pos_2.x, pos_2.y, 0.0f), player.sword_color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(pos_3.x, pos_3.y, 0.0f), player.sword_color, glm::vec2(0.5f, 0.5f));
	};

	//walls:
	draw_rectangle(glm::vec2(-court_radius.x - wall_radius, 0.0f), glm::vec2(wall_radius, court_radius.y + 2.0f * wall_radius), fg_color);
	draw_rectangle(glm::vec2(court_radius.x + wall_radius, 0.0f), glm::vec2(wall_radius, court_radius.y + 2.0f * wall_radius), fg_color);
	draw_rectangle(glm::vec2(0.0f, -court_radius.y - wall_radius), glm::vec2(court_radius.x, wall_radius), fg_color);
	draw_rectangle(glm::vec2(0.0f, court_radius.y + wall_radius), glm::vec2(court_radius.x, wall_radius), fg_color);


	//Player One
	draw_rectangle(playerOne.head, playerOne.head_radius, playerOne.head_color);
	draw_rectangle(playerOne.torso, playerOne.torso_radius, playerOne.torso_color);
	draw_angle_rectangle(playerOne.left_arm, playerOne.left_arm_radius, playerOne.left_arm_color, playerOne.left_arm_angle);
	draw_angle_rectangle(playerOne.right_arm, playerOne.right_arm_radius, playerOne.right_arm_color, playerOne.right_arm_angle);
	draw_rectangle(playerOne.left_leg, playerOne.left_leg_radius, playerOne.left_leg_color);
	draw_rectangle(playerOne.right_leg, playerOne.right_leg_radius, playerOne.right_leg_color);
	draw_sword(playerOne, sword_tip_length);

	//Player Two
	draw_rectangle(playerTwo.head, playerTwo.head_radius, playerTwo.head_color);
	draw_rectangle(playerTwo.torso, playerTwo.torso_radius, playerTwo.torso_color);
	draw_angle_rectangle(playerTwo.left_arm, playerTwo.left_arm_radius, playerTwo.left_arm_color, playerTwo.left_arm_angle);
	draw_angle_rectangle(playerTwo.right_arm, playerTwo.right_arm_radius, playerTwo.right_arm_color, playerTwo.right_arm_angle);
	draw_rectangle(playerTwo.left_leg, playerTwo.left_leg_radius, playerTwo.left_leg_color);
	draw_rectangle(playerTwo.right_leg, playerTwo.right_leg_radius, playerTwo.right_leg_color);
	draw_sword(playerTwo, sword_tip_length);

	//scores:
	glm::vec2 score_radius = glm::vec2(0.1f, 0.1f);
	for (uint32_t i = 0; i < left_score; ++i) {
		draw_rectangle(glm::vec2(-court_radius.x + (2.0f + 3.0f * i) * score_radius.x, court_radius.y + 2.0f * wall_radius + 2.0f * score_radius.y), score_radius, fg_color);
	}
	for (uint32_t i = 0; i < right_score; ++i) {
		draw_rectangle(glm::vec2(court_radius.x - (2.0f + 3.0f * i) * score_radius.x, court_radius.y + 2.0f * wall_radius + 2.0f * score_radius.y), score_radius, fg_color);
	}

	//------ compute court-to-window transform ------

	//compute area that should be visible:
	glm::vec2 scene_min = glm::vec2(
		-court_radius.x - 2.0f * wall_radius - padding,
		-court_radius.y - 2.0f * wall_radius - padding
	);
	glm::vec2 scene_max = glm::vec2(
		court_radius.x + 2.0f * wall_radius + padding,
		court_radius.y + 2.0f * wall_radius + 3.0f * score_radius.y + padding
	);

	//compute window aspect ratio:
	float aspect = drawable_size.x / float(drawable_size.y);
	//we'll scale the x coordinate by 1.0 / aspect to make sure things stay square.

	//compute scale factor for court given that...
	float scale = std::min(
		(2.0f * aspect) / (scene_max.x - scene_min.x), //... x must fit in [-aspect,aspect] ...
		(2.0f) / (scene_max.y - scene_min.y) //... y must fit in [-1,1].
	);

	glm::vec2 center = 0.5f * (scene_max + scene_min);

	//build matrix that scales and translates appropriately:
	glm::mat4 court_to_clip = glm::mat4(
		glm::vec4(scale / aspect, 0.0f, 0.0f, 0.0f),
		glm::vec4(0.0f, scale, 0.0f, 0.0f),
		glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
		glm::vec4(-center.x * (scale / aspect), -center.y * scale, 0.0f, 1.0f)
	);
	//NOTE: glm matrices are specified in *Column-Major* order,
	// so this matrix is actually transposed from how it appears.

	//also build the matrix that takes clip coordinates to court coordinates (used for mouse handling):
	clip_to_court = glm::mat3x2(
		glm::vec2(aspect / scale, 0.0f),
		glm::vec2(0.0f, 1.0f / scale),
		glm::vec2(center.x, center.y)
	);

	//---- actual drawing ----

	//clear the color buffer:
	glClearColor(bg_color.r / 255.0f, bg_color.g / 255.0f, bg_color.b / 255.0f, bg_color.a / 255.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	//use alpha blending:
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//don't use the depth test:
	glDisable(GL_DEPTH_TEST);

	//upload vertices to vertex_buffer:
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer); //set vertex_buffer as current
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertices[0]), vertices.data(), GL_STREAM_DRAW); //upload vertices array
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	//set color_texture_program as current program:
	glUseProgram(color_texture_program.program);

	//upload OBJECT_TO_CLIP to the proper uniform location:
	glUniformMatrix4fv(color_texture_program.OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(court_to_clip));

	//use the mapping vertex_buffer_for_color_texture_program to fetch vertex data:
	glBindVertexArray(vertex_buffer_for_color_texture_program);

	//bind the solid white texture to location zero:
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, white_tex);

	//run the OpenGL pipeline:
	glDrawArrays(GL_TRIANGLES, 0, GLsizei(vertices.size()));

	//unbind the solid white texture:
	glBindTexture(GL_TEXTURE_2D, 0);

	//reset vertex array to none:
	glBindVertexArray(0);

	//reset current program to none:
	glUseProgram(0);


	GL_ERRORS(); //PARANOIA: print errors just in case we did something wrong.

}
