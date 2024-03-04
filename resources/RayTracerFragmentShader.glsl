#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform int screenWidth;
uniform int screenHeight;

struct Camera {
	vec3 camPos;
	vec3 front;
	vec3 right;
	vec3 up;
	float halfH;
	float halfW;
	vec3 leftbottom;
	int LoopNum;
};
uniform struct Camera camera;

struct Ray {
	vec3 origin;
	vec3 direction;
};

uniform int spp;
int sample;

uniform float randOrigin;
uint wseed;
float rand(void);

struct Sphere {
	vec3 center;
	float radius;
	vec3 albedo;
	int materialIndex;
};
uniform Sphere sphere[6];

struct hitRecord {
	vec3 Normal;
	vec3 Pos;
	vec3 albedo;
	int materialIndex;
};
hitRecord rec;

// ����ֵ��ray���򽻵�ľ���
float hitSphere(Sphere s, Ray r);
bool hitWorld(Ray r);
vec3 shading(Ray r);

// ������ʷ֡�����������
uniform sampler2D historyTexture;

void main() {
	wseed = uint(randOrigin * float(6.95857) * (TexCoords.x * TexCoords.y));
	//if (distance(TexCoords, vec2(0.5, 0.5)) < 0.4)
	//	FragColor = vec4(rand(), rand(), rand(), 1.0);
	//else
	//	FragColor = vec4(0.0, 0.0, 0.0, 1.0);

	// ��ȡ��ʷ֡��Ϣ
	vec3 hist = texture(historyTexture, TexCoords).rgb;

	Ray cameraRay;
	cameraRay.origin = camera.camPos;
	cameraRay.direction = normalize(camera.leftbottom + (TexCoords.x * 2.0 * camera.halfW) * camera.right + (TexCoords.y * 2.0 * camera.halfH) * camera.up);

	vec3 curColor = shading(cameraRay);
	curColor = (1.0 / float(camera.LoopNum))*curColor + (float(camera.LoopNum - 1) / float(camera.LoopNum))*hist;
	FragColor = vec4(curColor, 1.0);

}


// ************ ��������� ************** //
float randcore(uint seed) {
	seed = (seed ^ uint(61)) ^ (seed >> uint(16));
	seed *= uint(9);
	seed = seed ^ (seed >> uint(4));
	seed *= uint(0x27d4eb2d);
	wseed = seed ^ (seed >> uint(15));
	return float(wseed) * (1.0 / 4294967296.0);
}
float rand() {
	return randcore(wseed);
}


// ********* ���г�������غ��� ********* // 

// ����ֵ��ray���򽻵�ľ���
float hitSphere(Sphere s, Ray r) {
	vec3 oc = r.origin - s.center;
	float a = dot(r.direction, r.direction);
	float b = 2.0 * dot(oc, r.direction);
	float c = dot(oc, oc) - s.radius * s.radius;
	float discriminant = b * b - 4 * a * c;
	if (discriminant > 0.0) {
		float dis = (-b - sqrt(discriminant)) / (2.0 * a);
		if (dis > 0.0) return dis;
		else return -1.0;
	}
	else return -1.0;
}

// ����ֵ��ray���򽻵�ľ���
bool hitWorld(Ray r) {
	float dis = 100000;
	bool hitAnything = false;
	int hitSphereIndex;
	for (int i = 0; i < 6; i++) {
		float dis_t = hitSphere(sphere[i], r);
		if (dis_t > 0 && dis_t < dis) {
			dis = dis_t;
			hitSphereIndex = i;
			hitAnything = true;
		}
	}
	if (hitAnything) {
		rec.Pos = r.origin + dis * r.direction;
		rec.Normal = normalize(r.origin + dis * r.direction - sphere[hitSphereIndex].center);
		rec.albedo = sphere[hitSphereIndex].albedo;
		rec.materialIndex = sphere[hitSphereIndex].materialIndex;
		return true;
	}
	else return false;
}

vec3 random_in_unit_sphere() {
	vec3 p;
	do {
		p = 2.0 * vec3(rand(), rand() ,rand()) - vec3(1, 1, 1);
	} while (dot(p, p) >= 1.0);
	return p;
}

vec3 diffuseReflection(vec3 Normal) {
	return normalize(Normal + random_in_unit_sphere());
}

vec3 metalReflection(vec3 rayIn, vec3 Normal) {
	return normalize(rayIn - 2 * dot(rayIn, Normal) * Normal + 0.35* random_in_unit_sphere());
}

vec3 mirrorReflection(vec3 rayIn, vec3 Normal) {
	return normalize(rayIn - 2 * dot(rayIn, Normal) * Normal + 0.01* random_in_unit_sphere());
}

vec3 shading(Ray r) {
	vec3 resultColor = vec3(0.0, 0.0, 0.0);
	for (int sample = 0; sample < spp; sample++){
		Ray tmpr = r;
		vec3 color = vec3(1.0, 1.0, 1.0);
		bool hitAnything = false;
		for (int i = 0; i < 20; i++) {
			if (hitWorld(tmpr)) {
				tmpr.origin = rec.Pos;
				if(rec.materialIndex == 0)
					tmpr.direction = diffuseReflection(rec.Normal);
				else if(rec.materialIndex == 1)
					tmpr.direction = metalReflection(tmpr.direction, rec.Normal);
				else if(rec.materialIndex == 2)
					tmpr.direction = mirrorReflection(tmpr.direction, rec.Normal);
				color *= rec.albedo;
				hitAnything = true;
			}
			else {
				break;
			}
		}
		if(!hitAnything) color = vec3(0.0, 0.0, 0.0);
		resultColor = resultColor + color;
	}
	resultColor = resultColor / spp;
	return resultColor;
}





