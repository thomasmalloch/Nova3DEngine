#include "Nova3DEngine.h"

using namespace nova;

Camera::Camera(float screen_width, float screen_height)
{
	position_ = { 0, 0, 0 };
	angle_ = 0;
	cos_angle_ = cosf(angle_);
	sin_angle_ = sinf(angle_);
	screen_width_ = screen_width;
	screen_height_ = screen_height;
	near_ = 0.1f;
	far_ = 1e5f;
	SetFOVRadians(Math::half_pi_); // 90 degrees
}

Camera::~Camera() 
{
}

void Camera::UpdateClippingPlanes() 
{
	float x1 = 10.f * cosf(fov_half_ + Math::half_pi_);
	float z1 = 10.f * sinf(fov_half_ + Math::half_pi_);
	float x2 = cosf(fov_half_ + Math::half_pi_);
	float z2 = sinf(fov_half_ + Math::half_pi_);
	Plane left = Plane({ x1, 0, z1 }, { -(z2 - z1), 0, x2 - x1  });

	
	x1 = cosf(-fov_half_ + Math::half_pi_);
	z1 = sinf(-fov_half_ + Math::half_pi_);
	x2 = 10.f * cosf(-fov_half_ + Math::half_pi_);
	z2 = 10.f * sinf(-fov_half_ + Math::half_pi_);
	Plane right = Plane({ x2, 0, z2 }, { -(z2 - z1), 0, x2 - x1 });
	Plane plane_near = Plane({ 0, 0, near_ }, {0, 0, 1});
	Plane plane_far = Plane({ 0, 0, 250 }, { 0, 0, -1 });

	//vFov = Mathf.Atan(Mathf.Tan(hFov / 2) / c.aspect) * 2;
	/*float vfov_half = atanf(tanf(fov_half_) / (screen_width_ / screen_height_));

	Plane top = Plane({ 0, 1, 1 }, { 0, -1, 1 });
	Plane bottom = Plane({ 0, -1, 1 }, { 0, 1, 1 });*/

	clipping_planes_.clear();

	//clipping_planes_.push_back(plane_far);
	clipping_planes_.push_back(right);
	clipping_planes_.push_back(left);
	
	clipping_planes_.push_back(plane_near);
}

void Camera::SetPosition(float x, float y, float z) 
{
	SetPosition({x, y, z});
}

void Camera::SetPosition(sf::Vector3f position) 
{
	position_ = position;
}

void Camera::SetAngle(float angle)
{
	angle_ = angle;
	cos_angle_ = cosf(angle);
	sin_angle_ = sinf(angle);
}

void Camera::SetNear(float camera_near) 
{
	near_ = camera_near;
	UpdateClippingPlanes();
}

void Camera::SetFar(float camera_far) 
{
	far_ = camera_far;
	UpdateClippingPlanes();
}

void Camera::SetFOVDegrees(float fov_degrees)
{
	SetFOVRadians(fov_degrees * Math::pi_ / 180.f);
}

void Camera::SetFOVRadians(float fov_rad)
{
	fov_ = fov_rad;
	fov_half_ = 0.5f * fov_rad;
	float scale = 1 / tanf(fov_half_);
	float aspect = screen_width_ / screen_height_;
	x_scale_ = 0.5 * screen_width_ * scale / std::min(1.f, aspect);
	y_scale_ = 0.5 * screen_height_ * scale / std::min(1.f, aspect);
	UpdateClippingPlanes();
}

float Camera::GetFOV()
{
	return fov_;
}

float Camera::GetFOVHalf()
{
	return fov_half_;
}

float Camera::GetAngle()
{
	return angle_;
}

float Camera::GetCosAngle()
{
	return cos_angle_;
}

float Camera::GetSinAngle()
{
	return sin_angle_;
}

const sf::Vector2f& Camera::GetScale()
{
	return { x_scale_, y_scale_ };
}

void Camera::SetCurrentNode(class Node* node)
{
	current_node_ = node;
}

class Node* Camera::GetCurrentNode()
{
	return current_node_;
}

const std::vector<Plane>& Camera::GetClippingPlanes() const
{	
	return clipping_planes_;
}

const sf::Vector3f& Camera::GetPosition() 
{
	return position_;
}

float Camera::GetNear() 
{
	return near_;
}

float Camera::GetFar() 
{
	return far_;
}

sf::Vector2f Camera::Project(sf::Vector3f p)
{
	return
	{
		0.5f * screen_width_ + x_scale_ * p.x / p.z,
		0.5f * screen_height_ + y_scale_ * p.y / p.z
	};
}

sf::Vector3f Camera::InverseProject(sf::Vector2f p, float z)
{
	return
	{
		(p.x - 0.5f * screen_width_) * z / x_scale_,
		(p.y - 0.5f * screen_height_) * z / y_scale_,
		z
	};
}