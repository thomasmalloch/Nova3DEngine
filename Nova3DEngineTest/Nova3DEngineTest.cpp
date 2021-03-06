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
public:
	class Player* player_;
	class Map* map_;

	class sf::Image* wall_texture_;
	class sf::Image* floor_texture_;
	class sf::Image* ceiling_texture_;

	void UserLoad() 
	{
		/*Wall *w = new Wall(-10, 32, -10, -32, sf::Color::White);
		
		Node *n = new Node();
		n->walls_.push_back(w);
		n->floor_height_ = 0;
		n->ceiling_height_ = 32;

		map_ = new Map();
		map_->nodes_.push_back(n);
		map_->player_angle_ = Math::pi_;
		map_->player_start_ = { 16, 0, 0};*/


		map_ = new Map("level1.map");
		map_->nodes_[1]->floor_height_ -= 10;
		map_->nodes_[1]->ceiling_height_ -= 10;

		map_->nodes_[4]->floor_height_ += 10;
		map_->nodes_[4]->ceiling_height_ += 10;


		wall_texture_ = new sf::Image();
		wall_texture_->loadFromFile("wall.png");
		
		floor_texture_ = new sf::Image();
		floor_texture_->loadFromFile("floor.png");

		ceiling_texture_ = new sf::Image();
		ceiling_texture_->loadFromFile("ceiling.png");

		// add texture to walls/floor/ceiling
		int tex_width = 16;
		int tex_height = 16;

		for (int i = 0; i < map_->nodes_.size(); i++)
		{
			float last_u = 0;
			for (int j = 0; j < map_->nodes_[i]->walls_.size(); j++)
			{
				Node* node = map_->nodes_[i];
				node->floor_texture_ = floor_texture_;
				node->ceiling_texture_ = ceiling_texture_;
				node->walls_[j]->wall_texture_ = wall_texture_;
				float dx = node->walls_[j]->p2_.x - node->walls_[j]->p1_.x;
				float dy = node->walls_[j]->p2_.y - node->walls_[j]->p1_.y;
				float len = sqrtf(dx * dx + dy * dy);
				node->walls_[j]->uv1_.x = last_u;
				node->walls_[j]->uv2_.x = last_u + len / tex_width;
				node->walls_[j]->texture_height_pixels_ = tex_height;
				last_u -= (int)last_u;
			}
		}

		LoadMap(map_);

		player_ = new Player();
		player_->position_ = map_->player_start_;
		player_->UpdateAngle(map_->player_angle_);
		player_->current_node_ = map_->nodes_[map_->player_node_index_];
		player_->height_ = 16;		
		AddPlayer(player_);

		this->GetCamera().SetFar(40.f);
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

		// near adjust
		if (input_manager.keys_[sf::Keyboard::Key::Numpad2])
		{
			this->GetCamera().SetNear(0.1f);
		}
		else if (input_manager.keys_[sf::Keyboard::Key::Numpad1])
		{
			float f_near = this->GetCamera().GetNear() - forward;
			this->GetCamera().SetNear(f_near);
		}
		else if (input_manager.keys_[sf::Keyboard::Key::Numpad3])
		{
			float f_near = this->GetCamera().GetNear() + forward;
			this->GetCamera().SetNear(f_near);
		}

		// far adjust
		if (input_manager.keys_[sf::Keyboard::Key::Numpad8])
		{
			this->GetCamera().SetFar(40.f);
		}
		else if (input_manager.keys_[sf::Keyboard::Key::Numpad7])
		{
			float f_far = this->GetCamera().GetFar() - forward;
			this->GetCamera().SetFar(f_far);
		}
		else if (input_manager.keys_[sf::Keyboard::Key::Numpad9])
		{
			float f_far = this->GetCamera().GetFar() + forward;
			this->GetCamera().SetFar(f_far);
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

	
	EngineTest* test = new EngineTest();

	test->Setup(960, 540, 2, false);
	test->Run();

	return 0;
}