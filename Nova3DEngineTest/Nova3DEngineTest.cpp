// Nova3DEngineTest.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include <Nova3DEngine.h>

using namespace nova;

class Player : public nova::IPlayer
{
	void Update(class IPlayer* player, std::vector<class IActor*> actors, const class UserInputManager* input_manager)
	{
	}

};

class EngineTest : public nova::NovaEngine
{
private:
	class Player* player_;
	class Map* map_;

	class Texture* wall_texture_;
	class Texture* floor_texture_;
	class Texture* ceiling_texture_;

	LPWSTR command_line_;

public:
	EngineTest(LPWSTR command_line) 
	{
		command_line_ = command_line;
	}

	void UserLoad() 
	{
		/*Wall *w1 = new Wall(-10, 32, -10, -32, sf::Color::White);
		Wall *w2 = new Wall(10, 32, 10, -32, sf::Color::White);
		Wall *w3 = new Wall(-10, -32, -10, -64, sf::Color::White);
		Wall *w4 = new Wall(10, -32, 10, -64, sf::Color::White);
		
		Node *n = new Node();
		n->walls_.push_back(w1);
		n->walls_.push_back(w2);
		n->walls_.push_back(w3);
		n->walls_.push_back(w4);
		n->floor_height_ = 0;
		n->ceiling_height_ = 32;

		map_ = new Map();
		map_->nodes_.push_back(n);
		map_->player_angle_ = Math::pi_;
		map_->player_start_ = { 16, 0, 0};*/

		//map_ = new Map("square.map");
		//map_ = new Map("octagon.map");
		//map_ = new Map("level1.map");
		//std::wcsstr(
		
		//char zoop[100] = { 0 };
		std::wstring wide(command_line_);
		std::string zoop(wide.begin(), wide.end());

		if (zoop.length() == 0)
			zoop = "level2.map";

		map_ = new Map(zoop);
		if (map_->nodes_.size() > 1) 
		{
			map_->nodes_[1]->floor_height_ -= 10;
			map_->nodes_[1]->ceiling_height_ -= 10;
		}

		if (map_->nodes_.size() > 4) 
		{
			map_->nodes_[4]->floor_height_ += 10;
			map_->nodes_[4]->ceiling_height_ += 10;
		}

		sf::Image wall_image;
		wall_image.loadFromFile("wall.png");
		wall_texture_ = new Texture(wall_image);

		sf::Image floor_image;
		floor_image.loadFromFile("floor.png");
		floor_texture_ = new Texture(floor_image);

		sf::Image ceiling_image;
		ceiling_image.loadFromFile("ceiling.png");
		ceiling_texture_ = new Texture(ceiling_image);

		// add texture to walls/floor/ceiling
		int plane_tex_width = 100;
		int plane_tex_height = 100;

		int wall_tex_width = 16;
		int wall_tex_height = 16;

		for (int i = 0; i < map_->nodes_.size(); i++)
		{
			Node* node = map_->nodes_[i];

			// assign node textures
			node->floor_texture_ = floor_texture_;
			node->ceiling_texture_ = ceiling_texture_;

			float last_u = 0;
			float top = FLT_MAX;
			float left = FLT_MAX;

			for (int j = 0; j < node->walls_.size(); j++)
			{				
				Wall* wall = node->walls_[j];

				// assign texture				
				wall->wall_texture_ = wall_texture_;

				// figure out wall texture coords
				float dx = wall->p2_.x - wall->p1_.x;
				float dy = wall->p2_.y - wall->p1_.y;
				float len = sqrtf(dx * dx + dy * dy);
				wall->uv1_.x = last_u;
				wall->uv2_.x = last_u + len / wall_tex_width;
				wall->texture_height_pixels_ = wall_tex_height;
				last_u -= (int)last_u;

				// figure out node texture offset coords
				if (wall->p1_.x < left)
					left = wall->p1_.x;
				if (wall->p2_.x < left)
					left = wall->p2_.x;
				if (wall->p1_.y < top)
					top = wall->p1_.y;
				if (wall->p2_.y < top)
					top = wall->p2_.y;
			}

			for (auto wall : node->walls_) 
			{
				node->plane_xy_.push_back(wall->p1_);
				//node->plane_xy_.push_back(wall->p2_);

				node->plane_uv_.push_back({ (wall->p1_.x - left) / plane_tex_width, (wall->p1_.y - top) / plane_tex_height });
				//node->plane_uv_.push_back({ (wall->p2_.x - left) / plane_tex_width, (wall->p2_.y - top) / plane_tex_height });
			}
		}

		LoadMap(map_);

		player_ = new Player();
		player_->position_ = map_->player_start_;
		player_->UpdateAngle(map_->player_angle_ + 0.01f);
		player_->current_node_ = map_->nodes_[map_->player_node_index_];
		player_->height_ = 16;		
		AddPlayer(player_);

		this->GetCamera().SetCurrentNode(player_->current_node_);
		this->GetCamera().SetPosition( { player_->position_.x, player_->position_.y, player_->position_.z + player_->height_ } );
		this->GetCamera().SetFOVRadians(Math::half_pi_);
		this->GetCamera().SetAngle(player_->angle_);
	}

	void UserUnload()
	{
		delete player_;
		delete map_;
		delete floor_texture_;
		delete ceiling_texture_;
		delete wall_texture_;
	}

	const std::string& Title() const
	{
		std::string blarg = "test";
		return blarg;
	}

	void UpdateWorker(sf::Time delta, class UserInputManager& input_manager)
	{
		float forward = (0.00004f * delta.asMicroseconds());
		float angle = (0.000002f * delta.asMicroseconds());

		// fov adjust
		if (input_manager.keys_[sf::Keyboard::Key::Numpad5])
		{
			this->GetCamera().SetFOVRadians(Math::half_pi_);
		}
		else if (input_manager.keys_[sf::Keyboard::Key::Numpad6]) 
		{
			float fov = this->GetCamera().GetFOV();
			fov -= angle;
			if (fov < FLT_EPSILON)
				fov = FLT_EPSILON;

			this->GetCamera().SetFOVRadians(fov);
		}
		else if (input_manager.keys_[sf::Keyboard::Key::Numpad4])
		{
			float fov = this->GetCamera().GetFOV();
			fov += angle;
			if (fov > Math::pi_)
				fov = Math::pi_;

			this->GetCamera().SetFOVRadians(fov);
		}

		// player movement
		if (input_manager.keys_[sf::Keyboard::Key::Left])
		{
			float f = this->player_->angle_ -= angle;
			this->player_->UpdateAngle(f);
			this->GetCamera().SetAngle(f);
		}

		if (input_manager.keys_[sf::Keyboard::Key::Right])
		{
			float f = this->player_->angle_ += angle;
			this->player_->UpdateAngle(f);
			this->GetCamera().SetAngle(f);
		}

		sf::Vector2f m = { 0,0 };
		bool movement_pressed = false;
		if (input_manager.keys_[sf::Keyboard::Key::W])
		{
			m.x += cosf(this->player_->angle_) * forward;
			m.y += sinf(this->player_->angle_) * forward;
			movement_pressed = true;
		}

		if (input_manager.keys_[sf::Keyboard::Key::S])
		{
			m.x -= cosf(this->player_->angle_) * forward;
			m.y -= sinf(this->player_->angle_) * forward;
			movement_pressed = true;
		}

		if (input_manager.keys_[sf::Keyboard::Key::D])
		{
			m.x += cosf(this->player_->angle_ + Math::half_pi_) * forward;
			m.y += sinf(this->player_->angle_ + Math::half_pi_) * forward;
			movement_pressed = true;
		}

		if (input_manager.keys_[sf::Keyboard::Key::A])
		{
			m.x -= cosf(this->player_->angle_ + Math::half_pi_) * forward;
			m.y -= sinf(this->player_->angle_ + Math::half_pi_) * forward;
			movement_pressed = true;
		}

		if (movement_pressed)
		{
			this->player_->position_ = sf::Vector3f(this->player_->position_.x + m.x, this->player_->position_.y + m.y, this->player_->position_.z);
			this->GetCamera().SetPosition( { player_->position_.x, player_->position_.y, player_->position_.z + player_->height_ } );
		}
	}

	IActor* ConvertActorFromID(const class IActor& actor) 
	{
		return nullptr;
	}
};


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{

	
	EngineTest* test = new EngineTest(lpCmdLine);

#if _DEBUG
	test->Setup(1280, 720, 2, false);
#else
	test->Setup(1280, 720, 1, false);
#endif

	test->Run();

	return 0;
}