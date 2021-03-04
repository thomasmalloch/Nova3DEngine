#include "Nova3DEngine.h"

using namespace nova;
using namespace std;

#pragma region Wall

Wall::Wall(float x1, float y1, float x2, float y2, int nodeIndex) :
	p1_(sf::Vector2f(x1, y1)),
	p2_(sf::Vector2f(x2, y2)),
	colour_(sf::Color::Red),
	texture_height_pixels_(1),
	node_index_(nodeIndex),
	wall_texture_(nullptr)
{
}

Wall::Wall(float x1, float y1, float x2, float y2, sf::Color colour) :
	p1_(sf::Vector2f(x1, y1)),
	p2_(sf::Vector2f(x2, y2)),
	colour_(colour),
	texture_height_pixels_(1),
	node_index_(-1),
	wall_texture_(nullptr)
{
}

Wall::~Wall()
{
}

#pragma endregion

#pragma region Node

Node::Node() :
	ceiling_height_(32),
	floor_height_(0),
	floor_texture_(nullptr),
	ceiling_texture_(nullptr)
{
}

Node::Node(std::vector<Wall*> walls) :
	walls_(walls),
	ceiling_height_(32),
	floor_height_(0),
	floor_texture_(nullptr),
	ceiling_texture_(nullptr)
{
}


Node::~Node()
{
}

#pragma endregion

#pragma region Map

Map::Map() :
	player_angle_(0),
	player_node_index_(0)
{
}

Map::Map(std::string filename) :
	player_angle_(0),
	player_node_index_(0),
	ambient_light_(0.2f)
{
	std::fstream file(filename);
	std::string line;
	while (std::getline(file, line))
	{
		if (line.length() == 0)
			continue;

		std::vector<std::string> words;
		std::size_t current = 0;
		while (true)
		{
			std::size_t index = line.find(' ', current);
			if (index == std::string::npos)
			{
				if (current != line.length())
				{
					std::string word = line.substr(current, line.length() - current);
					words.push_back(word);
				}

				break;
			}

			std::string word = line.substr(current, index - current);
			words.push_back(word);
			current = index + 1;
		}

		FileParseModeEnum mode = FileParseMode_Null;
		for (unsigned i = 0; i < words.size(); i++)
		{
			bool isFinishedWithLine = false;
			if (i == 0)
			{
				std::string word = words[0];
				if (word == "node")
				{
					nodes_.push_back(new class Node());
					mode = FileParseMode_Node;
				}
				else if (word == "portal")
				{
					mode = FileParseMode_Portal;
				}
				else if (word == "wall")
				{
					mode = FileParseMode_Wall;
				}
				else if (word == "player")
				{
					mode = FileParseMode_Player;
				}
				else if (word == "actor")
				{
					mode = FileParseMode_Actor;
				}
				else if (word == "map")
				{
					mode = FileParseMode_Map;
				}
				else
				{
					mode = FileParseMode_Null;
				}
			}
			else
			{
				switch (mode)
				{
				case FileParseMode_Node:
					nodes_[nodes_.size() - 1]->floor_height_ = (float)atof(words[i + 0].c_str());
					nodes_[nodes_.size() - 1]->ceiling_height_ = (float)atof(words[i + 1].c_str());
					isFinishedWithLine = true;
					break;
				case FileParseMode_Portal:
				{
					Wall* portal = new Wall(
						(float)atof(words[i + 0].c_str()) * 10.f,
						(float)atof(words[i + 1].c_str()) * 10.f,
						(float)atof(words[i + 2].c_str()) * 10.f,
						(float)atof(words[i + 3].c_str()) * 10.f,
						atoi(words[i + 4].c_str()));

					nodes_[nodes_.size() - 1]->walls_.push_back(portal);
					isFinishedWithLine = true;
					break;
				}
				case FileParseMode_Wall:
				{
					int argb = atoi(words[i + 4].c_str());
					sf::Color colour = sf::Color(
						((argb & 0x00FF0000) >> 16),
						((argb & 0x0000FF00) >> 8),
						((argb & 0x000000FF) >> 0),
						255);

					Wall* wall = new Wall(
						(float)atof(words[i + 0].c_str()) * 10.f,
						(float)atof(words[i + 1].c_str()) * 10.f,
						(float)atof(words[i + 2].c_str()) * 10.f,
						(float)atof(words[i + 3].c_str()) * 10.f,
						colour);

					nodes_[nodes_.size() - 1]->walls_.push_back(wall);
					isFinishedWithLine = true;
					break;
				}
				case FileParseMode_Player:
					this->player_start_.x = (float)atof(words[i + 0].c_str());
					this->player_start_.y = (float)atof(words[i + 1].c_str());
					this->player_start_.z = (float)atof(words[i + 2].c_str());
					this->player_angle_ = (float)atof(words[i + 3].c_str());
					this->player_node_index_ = atoi(words[i + 4].c_str());
					isFinishedWithLine = true;
					break;
				case FileParseMode_Actor:
				{
					float x = (float)atof(words[i + 0].c_str());
					float y = (float)atof(words[i + 1].c_str());
					float z = (float)atof(words[i + 2].c_str());
					int index = atoi(words[i + 3].c_str());
					float angle = 0;
					if (words.size() > i + 4)
						angle = (float)atof(words[i + 4].c_str());

					/*Sprite* sprite = new Sprite();
					sprite->position.x = x;
					sprite->position.y = y;
					sprite->position.z = z;
					sprite->actorTypeIndex = index;
					sprite->angle = angle;

					nodes[nodes.size() - 1]->temp.push_back(sprite);*/
					isFinishedWithLine = true;
					break;
				}
				case FileParseMode_Map:
				{
					if (words.size() > i)
						ambient_light_ = (float)atof(words[i].c_str());

					isFinishedWithLine = true;
					break;
				}
				case FileParseMode_Null:
				default:
					break;
				}
			}

			if (isFinishedWithLine)
				break;
		}
	}
}


Map::~Map()
{
}

#pragma endregion