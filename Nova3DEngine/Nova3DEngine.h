#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Audio.hpp>
#include <string>
#include <fstream>
#include <iostream>

namespace nova 
{

#pragma region Definitions

#pragma region Enums

	enum RenderModeEnum 
	{
		RenderMode_Tile,
		RenderMode_Portal,
	};

	enum FileParseModeEnum
	{
		FileParseMode_Null,
		FileParseMode_Portal,
		FileParseMode_Wall,
		FileParseMode_Player,
		FileParseMode_Node,
		FileParseMode_Actor,
		FileParseMode_Map,
	};

	enum TextureModeEnum
	{
		TextureMode_Tiled = 0,
		TextureMode_Stretched = 1,
	};

	enum EngineMessageEnum
	{
		// Engine Messages

		// User defined messages
		EngineMessage_User = 1000,
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

		static inline float Interpolate() 		
		{
			return 0; //x_start + ((end - start) / ())
		}

		static inline float Clamp(float min, float max, float num) 
		{
			if (num < min)
				return min;
			if (num > max)
				return max;

			return num;
		}

		static inline float CrossProduct(float x1, float y1, float x2, float y2)
		{
			return x1 * y2 - y1 * x2;
		}

		static inline float DotProduct(float x1, float y1, float x2, float y2)
		{
			return x1 * x2 + y1 * y2;
		}

		static inline const sf::Vector2f& const Intersect (float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4)
		{
			float d = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
			// If d is zero, there is no intersection
			if (d == 0) 
				return { 0, 0 };

			// Get the x and y
			float pre = (x1 * y2 - y1 * x2), post = (x3 * y4 - y3 * x4);
			float x = (pre * (x3 - x4) - (x1 - x2) * post) / d;
			float y = (pre * (y3 - y4) - (y1 - y2) * post) / d;

			// Return the point of intersection
			return {x, y};
		}
	};

	class Slope 
	{
	private:
		float slope_;

	public:
		inline Slope(float dy, float dx) 
		{
			slope_ = dy / dx;
		}

		inline float Interpolate(float start, float step) 
		{
			return start + slope_ * step;
		}
	};

	struct Matrix4x4
	{
		float m_[4][4] = { 0 };

		inline const sf::Vector3f& operator * (sf::Vector3f v)
		{
			sf::Vector3f output =
			{
				v.x * m_[0][0] + v.y * m_[1][0] + v.z * m_[2][0] + m_[3][0],
				v.x * m_[0][1] + v.y * m_[1][1] + v.z * m_[2][1] + m_[3][1],
				v.x * m_[0][2] + v.y * m_[1][2] + v.z * m_[2][2] + m_[3][2]
			};

			float w = v.x * m_[0][3] + v.y * m_[1][3] + v.z * m_[2][3] + m_[3][3];
			if (w != 0.0f)
				output /= w;

			return output;
		}

		inline const Matrix4x4& operator * (const Matrix4x4& other) 
		{
			Matrix4x4 result;
			for (int i = 0; i < 4; i++) 
			for (int j = 0; j < 4; j++)
			{
				result.m_[j][i] = 
					m_[j][0] * other.m_[0][i] +
					m_[j][1] * other.m_[1][i] +
					m_[j][2] * other.m_[2][i] +
					m_[j][3] * other.m_[3][i];
			}

			return result;
		}

	};

	class ProjectionMatrix : public Matrix4x4
	{
	public:
		inline ProjectionMatrix(int width, int height, int fov_degrees, float frutstrum_near, float frutstrum_far)
		{
			float fov = 1.0f / tanf(fov_degrees * 0.5f / 180.0f * Math::pi_);
			float aspect = (float)height / (float)width;

			m_[0][0] = aspect * fov;
			m_[1][1] = fov;
			m_[2][2] = frutstrum_far / (frutstrum_near - frutstrum_near);
			m_[2][3] = 1.0f;
			m_[3][2] = (-frutstrum_far * frutstrum_near) / (frutstrum_far - frutstrum_near);
			m_[3][3] = 0.0f;
		}

	};
#pragma endregion

#pragma region Actor

	class ICanMove 
	{
	public:
		inline void UpdateAngle(float angle)
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
	public:
		// public virtual methods
		virtual void Update(class IPlayer* player, std::vector<class IActor*> actors, class UserInputManager* input_manager) = 0;
		virtual sf::IntRect GetCurrentFrame() = 0;
		virtual void UpdateDirection(const sf::Vector2f& player_position) = 0;
		virtual bool GetIsBillboard() = 0;

		sf::Vector3f position_;
		sf::Vector2f size_;
		class sf::Image* image_;
		class Node* current_node_;
		int actor_type_index_;
	};

	class IPlayer : public IActor, public ICanMove
	{
	public:
		// constructor/destructor
		IPlayer();
		IPlayer(const sf::Vector3f& position, float angle);
		~IPlayer();

		// IActor
		virtual void Update(class IPlayer* player, std::vector<class IActor*> actors, class UserInputManager* input_manager) override {}
		virtual void UpdateDirection(const sf::Vector2f& player_position) override {}
		virtual sf::IntRect GetCurrentFrame() override
		{
			return { 0,0,0,0 };
		}

		virtual bool GetIsBillboard() override 
		{
			return false;
		}

		// public members
		class Node* node_;
		float pitch_;
		float height_;		
	};

#pragma endregion

#pragma region Renderers

	class IRenderer
	{
		
	};

#pragma region Portal Renderer

	class Frustum
	{
	public:
		sf::Vector2f far_[2];
		sf::Vector2f near_[2];
	};

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

		class Frustum* wall_clipping_planes_;
		class Frustum* pseudo_plane_frustrum_;

		void UpdateWallClippingPlanes();

	public:
		
		Camera(float screen_width, float screen_height);
		~Camera();
		void SetPosition(float x, float y, float z);
		void SetPosition(sf::Vector3f position);
		void SetAngle(float angle);
		void SetFOVDegrees(float fov_degrees);
		void SetFOVRadians(float fov_rad);
		void SetCurrentNode(class Node* node);

		const sf::Vector2f& GetScale();	
		class Node* GetCurrentNode();
		const class Frustum& GetWallClippingPlanes();
		float GetFOV();
		float GetFOVHalf();
		float GetAngle();
		float GetCosAngle();
		float GetSinAngle();
		const sf::Vector3f& GetPosition();

		sf::Vector2f Project(sf::Vector3f p);
		sf::Vector3f InverseProject(sf::Vector2f p, float z);
	};

	class Sprite
	{
	public:
		inline Sprite() 
			: angle_(0), actor_type_index(-1), position_(sf::Vector3f(0,0,0)) 
		{
		}

		inline ~Sprite() {}

		sf::Vector3f position_;
		int actor_type_index;
		float angle_;
	};

	struct Point3D
	{
		sf::Vector3f xyz_;
		sf::Vector3f uvw_;
	};

	struct Triangle 
	{
		class Point3D p_[3];
	};

	class Texture
	{
	private:
		sf::Uint8* data_;
		sf::Vector2u size_;

	public:
		inline Texture(const class sf::Image& image) 
		{
			size_ = image.getSize();
			data_ = new sf::Uint8[ size_.x * size_.y * 4 ];
			for (int x = 0; x < size_.x; x++) 
			{
				for (int y = 0; y < size_.y; y++) 
				{
					int offset = (x + y * size_.x) * 4;
					sf::Color colour = image.getPixel(x, y);
					data_[offset + 0] = colour.r;
					data_[offset + 1] = colour.g;
					data_[offset + 2] = colour.b;
					data_[offset + 3] = colour.a;
				}
			}
		}

		inline Texture(sf::Uint32 x, sf::Uint32 y)
		{
			size_ = { x, y };
			data_ = new sf::Uint8[size_.x * size_.y * 4];
		}

		inline ~Texture() 
		{
			delete[] data_;
		}

		inline const sf::Vector2u GetSize() const
		{
			return size_;		
		}

		inline const unsigned int GetWidth() const
		{
			return size_.x;
		}

		inline const unsigned int GetHeight() const
		{
			return size_.y;
		}

		inline const sf::Color GetPixel(unsigned int x, unsigned int y) const
		{
			int offset = (x + y * size_.x) * 4;
			return
			{
				data_[offset + 0],
				data_[offset + 1],
				data_[offset + 2],
				data_[offset + 3],
			};
		}

		inline void SetPixel(int x, int y, const sf::Color colour) 
		{
			int offset = (x + y * size_.x) * 4;
			data_[offset + 0] = colour.r;
			data_[offset + 1] = colour.g;
			data_[offset + 2] = colour.b;
			data_[offset + 3] = colour.a;
		}

		inline void Clear() 
		{
			memset(&data_[0], 0xff000000, size_.x * size_.y * 4);
		}

		inline const sf::Uint8* GetData() const
		{
			return data_;
		}
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
		const Texture* wall_texture_;
		float texture_height_pixels_;

		// Portal destination
		int node_index_;

		Wall(float x1, float y1, float x2, float y2, sf::Color colour);
		Wall(float x1, float y1, float x2, float y2, int nodeIndex);
		~Wall();

		inline bool IsPortal() { return (node_index_ >= 0); }	
	};

	class Node
	{
	public:
		const Texture* ceiling_texture_;
		const Texture* floor_texture_;
		std::vector<class Wall*> walls_;
		std::vector<class Sprite*> temp_;
		std::vector<class IActor*> actors_;

		sf::Vector2f plane_xy_quad_[4];
		sf::Vector2f plane_uv_quad_[4];

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
		float ambient_light_;
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
			for (int i = -1; i < sf::Keyboard::KeyCount; i++)
				keys_[(sf::Keyboard::Key)i] = false;

			for (int i = 0; i < sf::Mouse::ButtonCount; i++)
				mouse_buttons_[(sf::Mouse::Button)i] = false;
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
		bool Setup( int width, int height, int pixel_scale, bool fullscreen );
		// Runs the game
		void Run();
		// Ends the game
		void End();
		// Loads a map
		inline void LoadMap(class Map* map) 
		{
			current_map_ = map;
		}

		inline class Camera& GetCamera() 
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
		inline void AddPlayer(class IPlayer* player) 
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
		void RenderMap(const class Node& render_node, const class Node& last_node, const sf::Vector2f normalized_bounds[4], class Texture* pixels, class sf::RenderTexture& minimap);
		void RasterizeVerticalSlice(class Texture* pixels, const class sf::Color& colour, const sf::IntRect& screen_space, const sf::IntRect& portal_screen_space);
		void RasterizeVerticalSlice(class Texture* pixels, const class Texture& texture, const sf::FloatRect& uv, const sf::IntRect& screen_space, const sf::IntRect& portal_screen_space);
		void RasterizePseudoPlaneSlice(class Texture* pixels, float wall_z, const class Texture& floor_texture, const class Texture& ceiling_texture, const sf::IntRect& ceiling_screen_space, const sf::IntRect& wall_screen_space, const sf::IntRect& floor_screen_space);
		void RasterizePolygon(class Texture* pixels, const class Point3D points[], int vertex_count, const class Texture& texture);
		void RenderPlanes(class Texture* pixels, const class Node& render_node, const sf::Vector2f normalized_bounds[4]);

		void ClipTriangle(const class Point3D points[3], class Point3D new_points[10], int vertex_count);


		void RenderNodeActors(const class Node& node, class Texture* pixels, const sf::FloatRect& normalized_bounds);
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


		bool is_running_ = false;
	};
#pragma endregion

#pragma endregion




}
