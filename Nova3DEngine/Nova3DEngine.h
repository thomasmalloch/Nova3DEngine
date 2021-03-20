#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Audio.hpp>
#include <SFML/OpenGL.hpp>
#include <string>
#include <fstream>
#include <iostream>
#include <thread>
#include <algorithm>
#include <immintrin.h>
#include <xmmintrin.h>

namespace nova 
{

#pragma region Definitions

#pragma region Enums

	enum RenderModeEnum 
	{
		kRenderModeTile,
		kRenderModePortal,
	};

	enum FileParseModeEnum
	{
		kFileParseModeNull,
		kFileParseModePortal,
		kFileParseModeWall,
		kFileParseModePlayer,
		kFileParseModeNode,
		kFileParseModeActor,
		kFileParseModeMap,
	};

	enum TextureModeEnum
	{
		kTextureModeTiled = 0,
		kTextureModeStretched = 1,
	};

	enum EngineMessageEnum
	{
		// Engine Messages

		// User defined messages
		kEngineMessageUser = 1000,
	};

#pragma endregion

#pragma region Math

	class Math
	{
	public:
		static constexpr float pi_ = 3.141592654f;
		static constexpr float two_pi_ = 3.141592654f * 2.0f;
		static constexpr float quarter_pi_ = 3.141592654f / 4.0f;
		static constexpr float eighth_pi_ = 3.141592654f / 8.0f;
		static constexpr float half_pi_ = 3.141592654f / 2.0f;

		static float Interpolate() 		
		{
			return 0; //x_start + ((end - start) / ())
		}

		static float Clamp(const float min, const float max, const float num)
		{
			if (num < min)
				return min;
			if (num > max)
				return max;

			return num;
		}

		static float CrossProduct(const float x1, const float y1, const float x2, const float y2)
		{
			return x1 * y2 - y1 * x2;
		}

		static float DotProduct(const float x1, const float y1, const float x2, const float y2)
		{
			return x1 * x2 + y1 * y2;
		}

		static float DotProduct(const float x1, const float y1, const float z1, const float x2, const float y2, const float z2)
		{
			return x1 * x2 + y1 * y2 + z1 * z2;
		}

		static float DotProduct(const sf::Vector3f a, const sf::Vector3f b)
		{
			return a.x * b.x + a.y * b.y + a.z * b.z;
		}

		static float DotProduct(const sf::Vector2f a, const sf::Vector2f b)
		{
			return a.x * b.x + a.y * b.y;
		}

		static sf::Vector2f Intersect (const float x1, const float y1, const float x2, const float y2, const float x3, const float y3, const float x4, const float y4)
		{
			const auto d = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
			// If d is zero, there is no intersection
			if (d == 0) 
				return { 0, 0 };

			// Get the x and y
			const auto pre = (x1 * y2 - y1 * x2), post = (x3 * y4 - y3 * x4);
			const auto x = (pre * (x3 - x4) - (x1 - x2) * post) / d;
			const auto y = (pre * (y3 - y4) - (y1 - y2) * post) / d;

			// Return the point of intersection
			return { x, y };
		}

		static sf::Vector3f Normalize(const sf::Vector3f& p) 
		{
			const auto len = sqrtf(p.x * p.x + p.y * p.y + p.z * p.z);
			return { p.x / len, p.y / len, p.z / len };
		}

		static sf::Vector2f Normalize(const sf::Vector2f& p)
		{
			const auto len = sqrtf(p.x * p.x + p.y * p.y);
			return { p.x / len, p.y / len };
		}
	};

	class Plane 
	{
	private:
		sf::Vector3f normal_;
		sf::Vector3f point_;
		float a_;
		float b_;
		float c_;
		float d_;

	public:
		Plane(const sf::Vector3f point, const sf::Vector3f normal) 
		{
			point_ = point;
			normal_ = Math::Normalize(normal);
			a_ = normal_.x;
			b_ = normal_.y;
			c_ = normal_.z;
			d_ = -Math::DotProduct(point, normal);
		}

		float Distance(const sf::Vector3f p) const
		{
			return (a_ * p.x + b_ * p.y + c_ * p.z + d_);
		}

		sf::Vector3f Intersect(const sf::Vector3f p1, const sf::Vector3f p2) const
		{
			const auto ad = Math::DotProduct(p1, normal_);
			const auto bd = Math::DotProduct(p2, normal_);
			const auto t = (-d_ - ad) / (bd - ad);
			return (p2 - p1) * t + p1;
		}
	};

	class Slope 
	{
	private:
		float slope_;

	public:
		Slope(const float dy, const float dx) 
		{
			slope_ = dy / dx;
		}

		float Interpolate(const float start, const float step) const
		{
			return start + slope_ * step;
		}
	};

	struct Matrix4x4
	{
		float m[4][4] = { };

		sf::Vector3f operator * (sf::Vector3f v)
		{
			sf::Vector3f output =
			{
				v.x * m[0][0] + v.y * m[1][0] + v.z * m[2][0] + m[3][0],
				v.x * m[0][1] + v.y * m[1][1] + v.z * m[2][1] + m[3][1],
				v.x * m[0][2] + v.y * m[1][2] + v.z * m[2][2] + m[3][2]
			};

			const auto w = v.x * m[0][3] + v.y * m[1][3] + v.z * m[2][3] + m[3][3];
			if (w != 0.0f)
				output /= w;

			return output;
		}

	    Matrix4x4 operator * (const Matrix4x4& other) 
	    {
		    Matrix4x4 result;
		    for (auto i = 0; i < 4; i++) 
		    for (auto j = 0; j < 4; j++)
		    {
			    result.m[j][i] = 
				    m[j][0] * other.m[0][i] +
				    m[j][1] * other.m[1][i] +
				    m[j][2] * other.m[2][i] +
				    m[j][3] * other.m[3][i];
		    }

		    return result;
	    }
	};

	class ProjectionMatrix : public Matrix4x4
	{
	public:
		ProjectionMatrix(const int width, const int height, const  int fov_degrees, const float frutstrum_near, const float frutstrum_far)
		{
			const auto fov = 1.0f / tanf(fov_degrees * 0.5f / 180.0f * Math::pi_);
			const auto aspect = static_cast<float>(height) / static_cast<float>(width);

			m[0][0] = aspect * fov;
			m[1][1] = fov;
			// TODO could be a mistake here?
			m[2][2] = frutstrum_far / (frutstrum_far - frutstrum_near);
			m[2][3] = 1.0f;
			m[3][2] = (-frutstrum_far * frutstrum_near) / (frutstrum_far - frutstrum_near);
			m[3][3] = 0.0f;
		}
	};
#pragma endregion

#pragma region Actor

	class ICanMove 
	{
	public:
		void UpdateAngle(const float angle)
		{
			this->angle_ = angle;
			this->cos_ = std::cosf(angle);
			this->sin_ = std::sinf(angle);
			this->cos_90_ = std::cosf(angle + Math::half_pi_);
			this->sin_90_ = std::sinf(angle + Math::half_pi_);
			while (this->angle_ > Math::two_pi_)
				this->angle_ -= Math::two_pi_;
			while (this->angle_ < 0)
				this->angle_ += Math::two_pi_;
		}

		// public members
		float angle_;
		float sin_;
		float cos_;
		float sin_90_;
		float cos_90_;
	};

	// abstract class actor
	class IActor
	{
		sf::Vector3f position_;
		sf::Vector2f size_;
		class sf::Image *texture_;
		int actor_type_index_;

	public:
		// public virtual methods
		virtual void Update(class IPlayer* player, std::vector<class IActor*> actors, class UserInputManager* input_manager) = 0;
		virtual sf::IntRect GetCurrentFrame() = 0;
		virtual void UpdateDirection(const sf::Vector2f& player_position) = 0;
		virtual bool GetIsBillboard() = 0;
	};

#pragma endregion

#pragma region Renderers

#pragma region Portal Renderer

	class Camera
	{
	private:
		float x_scale_;
		float y_scale_;
		float screen_width_;
		float screen_height_;
		float fov_;
		float fov_half_;
		float near_;
		float far_;

		sf::Vector3f position_;
		float angle_;
		float cos_angle_;
		float sin_angle_;

		class Node* current_node_;
		class std::vector<Plane> clipping_planes_;

		void UpdateClippingPlanes();
	public:
		
		Camera(float screen_width, float screen_height);
		~Camera();
		void SetPosition(float x, float y, float z);
		void SetPosition(sf::Vector3f position);
		void SetAngle(float angle);
		void SetFOVDegrees(float fov_degrees);
		void SetFOVRadians(float fov_rad);
		void SetCurrentNode(class Node* node);
		void SetNear(float camera_near);
		void SetFar(float camera_far);

		sf::Vector2f GetScale() const;	
		class Node* GetCurrentNode() const;
		const std::vector<Plane>& GetClippingPlanes() const;
		float GetFOV() const;
		float GetFOVHalf() const;
		float GetAngle() const;
		float GetCosAngle() const;
		float GetSinAngle() const;
		float GetNear() const;
		float GetFar() const;
		const sf::Vector3f& GetPosition() const;

		sf::Vector2f Project(sf::Vector3f p) const;
		sf::Vector3f InverseProject(sf::Vector2f p, float z) const;
	};

	struct Point3D
	{		
		sf::Vector3f xyz;
		sf::Vector3f uvw;		
		sf::Vector3f original_position;
	};

	struct Triangle 
	{
		struct Point3D p[3];
	};

	class Wall
	{
	public:
		// Geometry
		sf::Vector2f p1_;
		sf::Vector2f p2_;

		// Texture Coordinates
		sf::Vector2f uv1_;
		sf::Vector2f uv2_;	

		// Texture and Colour information
		sf::Color colour_;
		const class sf::Image *wall_texture_;
		float texture_height_pixels_;

		// Portal destination
		int node_index_;

		Wall(float x1, float y1, float x2, float y2, sf::Color colour);
		Wall(float x1, float y1, float x2, float y2, int node_index);
		~Wall();

		bool IsPortal() const
		{
		    return (node_index_ >= 0);
		}	
	};

	class Node
	{
	public:
		const class sf::Image *ceiling_texture_;
		const class sf::Image *floor_texture_;
		std::vector<class Wall*> walls_;
		std::vector<class Sprite*> temp_;
		std::vector<class IActor*> actors_;

		std::vector<sf::Vector2f> plane_xy_;
		std::vector<sf::Vector2f> plane_uv_;

		float floor_height_;
		float ceiling_height_;

		Node();
		Node(std::vector<class Wall*> walls);
		~Node();
	};

	class Map
	{
	public:
		Map();
		Map(std::string filename);
		~Map();

		// player init
		sf::Vector3f player_start_;
		float player_angle_;
		int player_node_index_;

		// nodes
		std::vector<class Node*> nodes_;

		// ambient light
		float ambient_light_{};
	};

	class Light
	{
	public:
		sf::Vector3f position_;
		sf::Color colour_;
		float intensity_;

		Light(const sf::Color colour, const sf::Vector3f position)
			: position_(position), colour_(colour), intensity_(1.f)
		{
		}

		float GetDistance(const sf::Vector3f &p) const
		{
			return GetDistance(p.x, p.y, p.z);
		}

		float GetDistance(const float x, const float y, const float z) const
		{
			return std::sqrtf((position_.x - x) * (position_.x - x) + (position_.y - y) * (position_.y - y) + (position_.z - z) * (position_.z - z));
		}

		float GetDistanceSquared(const float x, const float y, const float z) const
		{
			return (position_.x - x) * (position_.x - x) + (position_.y - y) * (position_.y - y) + (position_.z - z) * (position_.z - z);
		}
	};

#pragma endregion

#pragma region Tile Renderer
#pragma endregion

#pragma endregion

#pragma region UI

	class IUserInterfaceControl 
	{
		std::vector<IUserInterfaceControl> children_;
		sf::IntRect bounds_;

		virtual void Paint() = 0;
	};

	class IUIPanel : public IUserInterfaceControl
	{
	};

	class IUILabel : public IUserInterfaceControl 
	{
	};

	class IUIButton : public IUserInterfaceControl
	{
	};
	

#pragma endregion

#pragma region Engine

	// This class is in charge of loading .map files into the game engine
	class MapLoader
	{

	};

	// This class is used to load and play music and sounds
	class SoundManager 
	{
	
	};

	// This class keeps track of user input
	class UserInputManager 
	{
	public:
		inline UserInputManager() :
			mouse_position_({ 0, 0 })
		{
			for (auto i = -1; i < sf::Keyboard::KeyCount; i++)
				keys_[static_cast<sf::Keyboard::Key>(i)] = false;

			for (auto i = 0; i < sf::Mouse::ButtonCount; i++)
				mouse_buttons_[static_cast<sf::Mouse::Button>(i)] = false;
		}

		std::map <sf::Keyboard::Key, bool> keys_;
		std::map <sf::Mouse::Button, bool> mouse_buttons_;
		sf::Vector2f mouse_position_;
	};

	// This class contains the main game loop, and will also contain all of the 
	class NovaEngine
	{
	public:
		// constructor/destructor
		NovaEngine();
		~NovaEngine();

		/////////////////////
		// Virtual Methods //
		/////////////////////
		// Function that the user can override that is called when the engine loads
		virtual void UserLoad() = 0;
		// Function that the user can override that is called when the engine unloads
		virtual void UserUnload() = 0;
		// The user defined title. Used for things like the window title.
		virtual const std::string& Title() const = 0;
		// Function that the user can override to add per frame updates
		virtual void UpdateWorker(sf::Time delta, class UserInputManager& manager) = 0;		

		////////////////////////////////////////////////
		// Engine Methods
		////////////////////////////////////////////////
		/// Function that sets up the game window
		bool Setup(int width, int height, int pixel_scale, bool fullscreen );
		// Runs the game
		void Run();
		// Ends the game
		void End();
		// Loads a map
		void LoadMap(class Map* map) 
		{
			current_map_ = map;
		}

		class Camera& GetCamera() const
		{
			return *camera_;
		}


		//////////////////////////////////////
		// Sound Manager Methods
		//////////////////////////////////////
		// Add music or sound. Returns the ID to the music/sound.
		int AddMusic(const std::string& filename);
		int AddSound(const std::string& filename);

		////////////////////////////////////////////////
		// Actor Methods
		////////////////////////////////////////////////
		// Add an actor manually
		void AddPlayer(class IPlayer* player) 
		{
			//this->player_ = player;
		}

		void AddActor(const class IActor& actor);
		// User overridden method for when the map loader wants to load an actor
		virtual class IActor* ConvertActorFromID(const class IActor& actor) = 0;

		////////////////////////////////////////////////
		// UI Methods
		////////////////////////////////////////////////
		void AddUI(const class IUserInterfaceControl& ui_root);

	protected:
	private:
		// private functions
		// renders the verticle walls of the map
		void RenderMap(const class Node& render_node, const class Node& last_node,
			const sf::Vector2f normalized_bounds[4],
			class sf::Image *pixels, class sf::RenderTexture &minimap);

		void RasterizeVerticalSlice(class sf::Image *pixels, const class sf::Image &texture,
			const Point3D points[2], const sf::IntRect& portal_screen_space);

		// renders the floors and ceiling
		void RasterizePolygon(class sf::Image *pixels, const struct Point3D points[], int vertex_count, const class sf::Image &texture);
		void RenderPlanes(class sf::Image *pixels, const class Node& render_node, class sf::RenderTexture& minimap);

		// plots a texel to the pixel texture.
		// also applies fog and lighting
		inline void DrawPixel(
			class sf::Image *pixels, int x, int y,
			float u, float v, const class sf::Image &texture,
			float z, float map_x, float map_y, float map_z);

		// returns the new number of verticies
		int ClipPolygon(struct Point3D points[], int num_points);

		void RenderNodeActors(const class Node& node, class sf::Image *pixels, const sf::FloatRect& normalized_bounds);
		void RenderUI();
		void UpdateActorPositions();
		void ProcessInput(const class sf::Event& event);		

		// members
		class UserInputManager* input_manager_;
		class Map* current_map_;
		class sf::RenderWindow* render_window_;
		class Camera* camera_;
		class IUserInterfaceControl* ui_root_;

		int window_width_;
		int window_height_;
		int pixel_scale_;
		int width_;
		int height_;
		bool is_fullscreen_;
		float *depth_buffer_;

		sf::Color fog_colour_;
		float fog_occlusion_distance_;
		float fog_density_;

		std::vector<Light*> lights_;
		std::vector<Light*> translated_lights_;

		bool is_running_ = false;
	};
#pragma endregion

#pragma endregion




}
