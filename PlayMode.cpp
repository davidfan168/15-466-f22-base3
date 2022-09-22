#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>

GLuint meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > get_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("scene.pnct"));
	meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > get_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("scene.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = get_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

Load< Sound::Sample > level_1_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("level_1.wav"));
});

Load< Sound::Sample > level_2_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("level_2.wav"));
});

Load< Sound::Sample > fire_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("fire.wav"));
});

Load< Sound::Sample > hurt_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("hurt.wav"));
});

PlayMode::PlayMode() : scene(*get_scene) {
	//get pointers to leg for convenience:
	for (auto &transform : scene.transforms) {
		if (transform.name == "Player") player = &transform;
		else if (transform.name == "Ground") ground = &transform;
	}
	if (player == nullptr) throw std::runtime_error("Player not found.");
	if (ground == nullptr) throw std::runtime_error("Ground not found.");

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

	//start music loop playing:
	// background_music = Sound::loop(*level_1_sample, 1.0f, 0);
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		} else if (evt.key.keysym.sym == SDLK_a) {
			left_btn_pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right_btn_pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up_btn_pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down_btn_pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
			space_btn_down = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			left_btn_pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right_btn_pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up_btn_pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down_btn_pressed = false;
			return true;
		}
	}

	return false;
}

void PlayMode::spawn_bullet() {
	Bullet bullet;
	float x_coord = 0, y_coord = 0;
	// up, down, left, right
	int direction = rand() % 4;
	if (direction == 0) {
		float rand_val = ((float) (rand() % 100)) / 100;
		y_coord = min_y + rand_val * (max_y - min_y);
		x_coord = min_x;
		bullet.direction = glm::vec3(1, 0, 0);
	}
	if (direction == 1) {
		float rand_val = ((float) (rand() % 100)) / 100;
		y_coord = min_y + rand_val * (max_y - min_y);
		x_coord = max_x;
		bullet.direction = glm::vec3(-1, 0, 0);
	}
	if (direction == 2) {
		float rand_val = ((float) (rand() % 100)) / 100;
		y_coord = max_y;
		x_coord = min_x + rand_val * (max_x - min_x);
		bullet.direction = glm::vec3(0, -1, 0);
	}
	if (direction == 3) {
		float rand_val = ((float) (rand() % 100)) / 100;
		y_coord = min_y;
		x_coord = min_x + rand_val * (max_x - min_x);
		bullet.direction = glm::vec3(0, 1, 0);
	}
	
	Scene::Transform *transform = new Scene::Transform;
	transform->position = glm::vec3(x_coord, y_coord, 1);
	transform->name = "Bullet";
	transform->scale = glm::vec3(0.3f, 0.3f, 0.1f);

	bullet.transform = transform;

	// add to drawables
	{
		Mesh const &mesh = get_meshes->lookup("Bullet");

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;
	}
	bullets.emplace_back(bullet);
}

void PlayMode::update(float elapsed) {

	if (score > 1000) {
		level = 2;
	}
	else {
		level = 1;
	}

	constexpr float music_duration = 9.7f;
	static float time_passed = 10.0f;

	time_passed += elapsed;
	if (time_passed >= music_duration) {
		Sound::play(*level_1_sample, 1.0f, 0);
		if (partial_counter < 24) {
			counter = 0;
		}
		partial_counter = 0;
		time_passed = 0;
	}
	// TODO implement level 2

	// movement
	{
	constexpr float speed = 5.0f;

	// moving diagonally is faster, but that is desired behavior
	if (up_btn_pressed) {
		player->position.x -= speed * elapsed;
		glm::vec3 angle = glm::vec3(0.0f, 0.0f, -M_PI / 2);
		player->rotation = glm::quat(angle);
	}
	if (down_btn_pressed) {
		player->position.x += speed * elapsed;
		glm::vec3 angle = glm::vec3(0.0f, 0.0f, M_PI / 2);
		player->rotation = glm::quat(angle);
	}
	if (left_btn_pressed) {
		player->position.y -= speed * elapsed;
		glm::vec3 angle = glm::vec3(0.0f, 0.0f, 0.0f);
		player->rotation = glm::quat(angle);
	}
	if (right_btn_pressed) {
		player->position.y += speed * elapsed;
		glm::vec3 angle = glm::vec3(0.0f, 0.0f, M_PI);
		player->rotation = glm::quat(angle);
	}

	// clamp the player within bounds
	if (player->position.x > max_x) {
		player->position.x = max_x;
	}
	if (player->position.y > max_y) {
		player->position.y = max_y;
	}
	if (player->position.x < min_x) {
		player->position.x = min_x;
	}
	if (player->position.y < min_y) {
		player->position.y = min_y;
	}
	}

	if (space_btn_down) {
		spawn_bullet();
		Sound::play(*fire_sample, 0.5f, 0);
		counter++;
		partial_counter++;
		score += counter * 10;
		space_btn_down = false;
	}

	constexpr float bullet_speed = 4.0f;
	for (Bullet bullet : bullets) {
		bullet.transform->position += bullet.direction * bullet_speed * elapsed;
		glm::vec3 bullet_pos = bullet.transform->position;
		glm::vec3 player_pos = player->position;
		float dist = sqrt(pow(bullet_pos.x - player_pos.x, 2) + pow(bullet_pos.y - player_pos.y, 2));
		// bullet hit
		if (dist < 0.4f) {
			Sound::play(*hurt_sample, 0.5f, 0);
			score = std::max(0, score - 1000);
		}
	}
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	scene.draw(*camera);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		constexpr float H = 0.09f;
		lines.draw_text("Move your character using WASD",
			glm::vec3(-aspect + 0.2f * H, -1.0 + 1.7f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		lines.draw_text("Try to press Space according to the beat",
			glm::vec3(-aspect + 0.2f * H, -1.0 + 0.5f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		lines.draw_text("SCORE: " + std::to_string(score),
			glm::vec3(-aspect + 0.2f * H, 1 - 1.5f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
	}
	GL_ERRORS();
}
