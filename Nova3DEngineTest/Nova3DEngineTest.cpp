// Nova3DEngineTest.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include <Nova3DEngine.h>

using namespace nova;

class EngineTest : public nova::NovaEngine
{
private:
	//class Player *player_;
	class Map *map_;

	class sf::Image *wall_texture_;
	class sf::Image *floor_texture_;
	class sf::Image *ceiling_texture_;

	std::string command_line_;
	int player_height_ = 16;

public:
	EngineTest(const std::string command_line) :
        map_(nullptr),
        wall_texture_(nullptr),
        floor_texture_(nullptr),
        ceiling_texture_(nullptr)
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
		

		if (command_line_.length() == 0)
			command_line_ = "lighttest.map";

		map_ = new Map(command_line_);

		wall_texture_ = new sf::Image();
		wall_texture_->loadFromFile("wall.png");

		floor_texture_ = new sf::Image();
		floor_texture_->loadFromFile("floor.png");

		ceiling_texture_ = new sf::Image();
		ceiling_texture_->loadFromFile("ceiling.png");

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

		this->GetCamera().SetCurrentNode(map_->nodes_[map_->player_node_index_]);
		this->GetCamera().SetPosition( { map_->player_start_.x, map_->player_start_.y, map_->player_start_.z + player_height_ } );
		this->GetCamera().SetFOVRadians(Math::half_pi_);
		this->GetCamera().SetAngle(map_->player_angle_ + 0.001f);
	}

	void UserUnload()
	{
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
		const auto forward = (0.00004f * delta.asMicroseconds());
		const auto angle = (0.000002f * delta.asMicroseconds());

		// fov adjust
		if (input_manager.keys_[sf::Keyboard::Key::Numpad5])
		{
			this->GetCamera().SetFOVRadians(Math::half_pi_);
		}
		else if (input_manager.keys_[sf::Keyboard::Key::Numpad6]) 
		{
			auto fov = this->GetCamera().GetFOV();
			fov -= angle;
			if (fov < FLT_EPSILON)
				fov = FLT_EPSILON;

			this->GetCamera().SetFOVRadians(fov);
		}
		else if (input_manager.keys_[sf::Keyboard::Key::Numpad4])
		{
			auto fov = this->GetCamera().GetFOV();
			fov += angle;
			if (fov > Math::pi_)
				fov = Math::pi_;

			this->GetCamera().SetFOVRadians(fov);
		}

		// player movement
		if (input_manager.keys_[sf::Keyboard::Key::Left])
		{
			auto f = this->GetCamera().GetAngle();
			f -= angle;
			this->GetCamera().SetAngle(f);
		}

		if (input_manager.keys_[sf::Keyboard::Key::Right])
		{
			auto f = this->GetCamera().GetAngle();
			f += angle;
			this->GetCamera().SetAngle(f);
		}

		sf::Vector3f m = { 0,0, 0 };
		auto movement_pressed = false;
		if (input_manager.keys_[sf::Keyboard::Key::W])
		{
			m.x += this->GetCamera().GetCosAngle() * forward;
			m.y += this->GetCamera().GetSinAngle() * forward;
			movement_pressed = true;
		}

		if (input_manager.keys_[sf::Keyboard::Key::S])
		{
			m.x -= this->GetCamera().GetCosAngle() * forward;
			m.y -= this->GetCamera().GetSinAngle() * forward;
			movement_pressed = true;
		}

		if (input_manager.keys_[sf::Keyboard::Key::D])
		{
			m.x += cosf(this->GetCamera().GetAngle() + Math::half_pi_) * forward;
			m.y += sinf(this->GetCamera().GetAngle() + Math::half_pi_) * forward;
			movement_pressed = true;
		}

		if (input_manager.keys_[sf::Keyboard::Key::A])
		{
			m.x -= cosf(this->GetCamera().GetAngle() + Math::half_pi_) * forward;
			m.y -= sinf(this->GetCamera().GetAngle() + Math::half_pi_) * forward;
			movement_pressed = true;
		}

		if (movement_pressed)
		{
			//this->player_->position_ = sf::Vector3f(this->player_->position_.x + m.x, this->player_->position_.y + m.y, this->player_->position_.z);
			const auto postion = this->GetCamera().GetPosition() + m;
			this->GetCamera().SetPosition(postion);
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

	//auto square = Math::FastSquare(25);

	std::wstring wide(lpCmdLine);
	std::string zoop(wide.begin(), wide.end());
	auto test = EngineTest(zoop);

#if _DEBUG
	test.Setup(1280, 720, 3, false);
#else
	test.Setup(1280, 720, 2, false);
#endif

	test.Run();
	return 0;
}