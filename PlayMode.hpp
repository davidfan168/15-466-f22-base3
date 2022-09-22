#include "Mode.hpp"

#include "Scene.hpp"
#include "Sound.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	//input tracking:
	bool space_btn_down = false;
	bool left_btn_pressed = false, right_btn_pressed = false, 
		 down_btn_pressed = false, up_btn_pressed = false;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	Scene::Transform *player = nullptr;
	Scene::Transform *ground = nullptr;

	// std::shared_ptr< Sound::PlayingSample > background_music;
	
	Scene::Camera *camera = nullptr;

	static constexpr float min_x = -3.88f;
	static constexpr float min_y = -4.61f;
	static constexpr float max_x = 4.0f;
	static constexpr float max_y = 3.53f;

	int score = 0;
	int level = 1;
	int counter = 0;
	int partial_counter = 0;
	
	struct Bullet {
		Scene::Transform *transform = nullptr;
		glm::vec3 direction;
	};
	void spawn_bullet();
	std::vector<Bullet> bullets;
};
