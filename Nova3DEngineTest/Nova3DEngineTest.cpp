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
		int tex_width = 100;
		int tex_height = 100;

		for (int i = 0; i < map_->nodes_.size(); i++)
		{
			Node* node = map_->nodes_[i];

			// assign node textures
			node->floor_texture_ = floor_texture_;
			node->ceiling_texture_ = ceiling_texture_;

			float last_u = 0;
			float top = FLT_MAX;
			float left = FLT_MAX;
			float right = FLT_MIN;
			float bottom = FLT_MIN;

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
				wall->uv2_.x = last_u + len / tex_width;
				wall->texture_height_pixels_ = tex_height;
				last_u -= (int)last_u;

				// figure out node plane coords
				if (wall->p1_.x < left)
					left = wall->p1_.x;
				if (wall->p2_.x < left)
					left = wall->p2_.x;
				if (wall->p1_.x > right)
					right = wall->p1_.x;
				if (wall->p2_.x > right)
					right = wall->p2_.x;

				if (wall->p1_.y < top)
					top = wall->p1_.y;
				if (wall->p2_.y < top)
					top = wall->p2_.y;
				if (wall->p1_.y > bottom)
					bottom = wall->p1_.y;
				if (wall->p2_.y > bottom)
					bottom = wall->p2_.y;
			}

			node->plane_xy_quad_[0] = { left, top };
			node->plane_xy_quad_[1] = { right, top };
			node->plane_xy_quad_[2] = { right, bottom };
			node->plane_xy_quad_[3] = { left, bottom };
			
			node->plane_uv_quad_[0] = { 0, 0 };
			node->plane_uv_quad_[1] = { (node->plane_xy_quad_[1].x - node->plane_xy_quad_[0].x) / tex_width, 0 };
			node->plane_uv_quad_[2] = { node->plane_uv_quad_[1].x, (node->plane_xy_quad_[3].y - node->plane_xy_quad_[0].y) / tex_height };
			node->plane_uv_quad_[3] = { 0, node->plane_uv_quad_[2].y };
		}

		LoadMap(map_);

		player_ = new Player();
		player_->position_ = map_->player_start_;
		player_->UpdateAngle(map_->player_angle_);
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

	
	EngineTest* test = new EngineTest();

	test->Setup(1280, 720, 1, false);
	test->Run();

	return 0;
}