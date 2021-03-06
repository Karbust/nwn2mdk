// Copyright 2017 Jose M. Arbos
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <string.h>

#include "mdb_file.h"

template <typename T>
static void read(std::istream& in, T& x)
{
	in.read((char*)&x, sizeof(T));
}

template <typename T>
static void read(std::istream& in, std::vector<T>& v)
{
	in.read((char*)v.data(), sizeof(T) * v.size());
}

template <typename T>
static void write(std::ostream& out, T& x)
{
	out.write((char*)&x, sizeof(T));
}

template <typename T>
static void write(std::ostream& out, std::vector<T>& v)
{
	out.write((char*)v.data(), sizeof(T) * v.size());
}

MDB_file::Walk_mesh_material MDB_file::walk_mesh_materials[] = {
	"w_Nonwalk", 0, Vector3<float>(1, 0, 0),
	"w_Dirt", 0x9, Vector3<float>(0.29f, 0.18f, 0.07f),
	"w_Grass", 0x11, Vector3<float>(0, 0.14f, 0),
	"w_Stone", 0x21, Vector3<float>(0.22f, 0.22f, 0.22f),
	"w_Wood", 0x41, Vector3<float>(1, 0.9f, 0.17f),
	"w_Carpet", 0x81, Vector3<float>(0.25f, 0, 0.25f),
	"w_Metal", 0x101, Vector3<float>(0.82f, 0.82f, 1),
	"w_Swamp", 0x201, Vector3<float>(0.41f, 0.67f, 0),
	"w_Mud", 0x401, Vector3<float>(0.25f, 0.07f, 0),
	"w_Leaves", 0x801, Vector3<float>(0.03f, 0.05f, 0),
	"w_Water", 0x1001, Vector3<float>(0.25f, 0.59f, 1),
	"w_Puddles", 0x2001, Vector3<float>(0.6f, 0.65f, 0.83f)
};

MDB_file::MDB_file()
{
	is_good_ = true;

	memcpy(header.signature, "NWN2", 4);
	header.major_version = 1;
	header.minor_version = 12;
	header.packet_count = 0;
}

MDB_file::MDB_file(const char* filename)
{
	is_good_ = false;

	std::ifstream in(filename, std::ios::in | std::ios::binary);
	if (!in) {
		error_str_ = "can't open file";
		return;
	}

	read(in);

}

MDB_file::MDB_file(std::istream& in)
{
	read(in);
}

void MDB_file::add_packet(std::unique_ptr<Packet> packet)
{
	if(!packet)
		return;

	Packet_key packet_key;
	memcpy(packet_key.type, packet->type_str(), 4);
	packet_key.offset = 0;
	packet_keys.push_back(packet_key);

	packets.push_back(move(packet));

	++header.packet_count;
}

const char* MDB_file::error_str() const
{
	return error_str_.c_str();
}

uint16_t MDB_file::major_version() const
{
	return header.major_version;
}

uint16_t MDB_file::minor_version() const
{
	return header.minor_version;
}

MDB_file::Packet* MDB_file::packet(uint32_t packet_index) const
{
	if (packet_index >= packet_keys.size())
		return nullptr;

	return packets[packet_index].get();
}

uint32_t MDB_file::packet_count() const
{
	return header.packet_count;
}

void MDB_file::read(std::istream& in)
{
	is_good_ = false;

	::read(in, header);

	if (strncmp(header.signature, "NWN2", 4) != 0) {
		error_str_ = "invalid file type";
		return;
	}

	packet_keys.resize(header.packet_count);
	::read(in, packet_keys);

	read_packets(in);

	is_good_ = true;
}

void MDB_file::read_packets(std::istream& in)
{
	for (auto& packet_key : packet_keys)
		read_packet(packet_key, in);
}

void MDB_file::read_packet(Packet_key& packet_key, std::istream& in)
{
	in.seekg(packet_key.offset);

	if (strncmp(packet_key.type, "COL2", 4) == 0)
		packets.emplace_back(new Collision_mesh(in));
	else if (strncmp(packet_key.type, "COL3", 4) == 0)
		packets.emplace_back(new Collision_mesh(in));
	else if (strncmp(packet_key.type, "COLS", 4) == 0)
		packets.emplace_back(new Collision_spheres(in));
	else if (strncmp(packet_key.type, "HAIR", 4) == 0)
		packets.emplace_back(new Hair(in));
	else if (strncmp(packet_key.type, "HELM", 4) == 0)
		packets.emplace_back(new Helm(in));
	else if (strncmp(packet_key.type, "HOOK", 4) == 0)
		packets.emplace_back(new Hook(in));
	else if (strncmp(packet_key.type, "RIGD", 4) == 0)
		packets.emplace_back(new Rigid_mesh(in));
	else if (strncmp(packet_key.type, "SKIN", 4) == 0)
		packets.emplace_back(new Skin(in));
	else if (strncmp(packet_key.type, "WALK", 4) == 0)
		packets.emplace_back(new Walk_mesh(in));
	else
		packets.push_back(nullptr);
}

void MDB_file::save(const char* filename)
{
	header.packet_count = packets.size();

	uint32_t offset =
	    sizeof(Header) + sizeof(Packet_key) * packet_keys.size();

	for(unsigned i = 0; i < packet_keys.size(); ++i) {
		packet_keys[i].offset = offset;
		offset += packets[i]->packet_size();
	}

	std::ofstream out(filename, std::ios::binary);
	out.write((char*)&header, sizeof(Header));
	out.write((char*)packet_keys.data(),
	          sizeof(Packet_key) * packet_keys.size());

	for(unsigned i = 0; i < packets.size(); ++i)
		packets[i]->write(out);
}

MDB_file::operator bool() const
{
	return is_good_;
}

const char* MDB_file::Packet::type_str() const
{
	switch(type) {
	case MDB_file::COL2:
		return "COL2";
	case MDB_file::COL3:
		return "COL3";
	case MDB_file::COLS:
		return "COLS";
	case MDB_file::HAIR:
		return "HAIR";
	case MDB_file::HELM:
		return "HELM";
	case MDB_file::HOOK:
		return "HOOK";
	case MDB_file::RIGD:
		return "RIGD";
	case MDB_file::SKIN:
		return "SKIN";
	case MDB_file::TRRN:
		return "TRRN";
	case MDB_file::WALK:
		return "WALK";
	}

	return "UNKNOWN";
}

MDB_file::Collision_mesh::Collision_mesh(Packet_type t)
{
	type = t;
	memcpy(header.type, type_str(), 4);
	header.packet_size = 0;
	memset(header.name, 0, sizeof(header.name));
	memset(header.material.diffuse_map_name, 0, sizeof(header.material.diffuse_map_name));
	memset(header.material.normal_map_name, 0, sizeof(header.material.normal_map_name));
	memset(header.material.tint_map_name, 0, sizeof(header.material.tint_map_name));
	memset(header.material.glow_map_name, 0, sizeof(header.material.glow_map_name));
	header.material.diffuse_color = Vector3<float>(1, 1, 1);
	header.material.specular_color = Vector3<float>(1, 1, 1);
	header.material.specular_level = 1;
	header.material.specular_power = 1;
	header.material.flags = 0;
	header.vertex_count = 0;
	header.face_count = 0;
}

MDB_file::Collision_mesh::Collision_mesh(std::istream& in)
{
	read(in);
}

uint32_t MDB_file::Collision_mesh::packet_size()
{
	return sizeof(Collision_mesh_header) +
	       sizeof(Collision_mesh_vertex) * verts.size() +
	       sizeof(Face) * faces.size();
}

void MDB_file::Collision_mesh::read(std::istream& in)
{
	::read(in, header);

	if(strncmp(header.type, "COL2", 4) == 0)
		type = COL2;
	else
		type = COL3;

	verts.resize(header.vertex_count);
	::read(in, verts);

	faces.resize(header.face_count);
	::read(in, faces);
}

void MDB_file::Collision_mesh::write(std::ostream& out)
{
	header.packet_size = packet_size() - sizeof(Packet_header);
	header.vertex_count = verts.size();
	header.face_count = faces.size();

	::write(out, header);
	::write(out, verts);
	::write(out, faces);
}

MDB_file::Rigid_mesh::Rigid_mesh()
{
	type = RIGD;
	memcpy(header.type, type_str(), 4);
	header.packet_size = 0;
	memset(header.name, 0, sizeof(header.name));
	memset(header.material.diffuse_map_name, 0, sizeof(header.material.diffuse_map_name));
	memset(header.material.normal_map_name, 0, sizeof(header.material.normal_map_name));
	memset(header.material.tint_map_name, 0, sizeof(header.material.tint_map_name));
	memset(header.material.glow_map_name, 0, sizeof(header.material.glow_map_name));
	header.material.diffuse_color = Vector3<float>(1, 1, 1);
	header.material.specular_color = Vector3<float>(1, 1, 1);
	header.material.specular_level = 1;
	header.material.specular_power = 1;
	header.material.flags = 0;
	header.vertex_count = 0;
	header.face_count = 0;
}

MDB_file::Rigid_mesh::Rigid_mesh(std::istream& in)
{
	read(in);
}

uint32_t MDB_file::Rigid_mesh::packet_size()
{
	return sizeof(Rigid_mesh_header) +
	       sizeof(Rigid_mesh_vertex) * verts.size() +
	       sizeof(Face) * faces.size();
}

void MDB_file::Rigid_mesh::read(std::istream& in)
{
	type = RIGD;

	::read(in, header);

	verts.resize(header.vertex_count);
	::read(in, verts);

	faces.resize(header.face_count);
	::read(in, faces);
}

void MDB_file::Rigid_mesh::write(std::ostream& out)
{
	header.packet_size = packet_size() - sizeof(Packet_header);
	header.vertex_count = verts.size();
	header.face_count = faces.size();

	::write(out, header);
	::write(out, verts);
	::write(out, faces);
}

MDB_file::Skin::Skin()
{
	type = SKIN;
	memcpy(header.type, type_str(), 4);
	header.packet_size = 0;
	memset(header.name, 0, sizeof(header.name));
	memset(header.skeleton_name, 0, sizeof(header.skeleton_name));
	memset(header.material.diffuse_map_name, 0, sizeof(header.material.diffuse_map_name));
	memset(header.material.normal_map_name, 0, sizeof(header.material.normal_map_name));
	memset(header.material.tint_map_name, 0, sizeof(header.material.tint_map_name));
	memset(header.material.glow_map_name, 0, sizeof(header.material.glow_map_name));
	header.material.diffuse_color = Vector3<float>(1, 1, 1);
	header.material.specular_color = Vector3<float>(1, 1, 1);
	header.material.specular_level = 1;
	header.material.specular_power = 1;
	header.material.flags = 0;
	header.vertex_count = 0;
	header.face_count = 0;
}

MDB_file::Skin::Skin(std::istream& in)
{
	read(in);
}

uint32_t MDB_file::Skin::packet_size()
{
	return sizeof(Skin_header) +
	       sizeof(Skin_vertex) * verts.size() +
	       sizeof(Face) * faces.size();
}

void MDB_file::Skin::read(std::istream& in)
{
	type = SKIN;

	::read(in, header);

	verts.resize(header.vertex_count);
	::read(in, verts);

	faces.resize(header.face_count);
	::read(in, faces);
}

void MDB_file::Skin::write(std::ostream& out)
{
	header.packet_size = packet_size() - sizeof(Packet_header);
	header.vertex_count = verts.size();
	header.face_count = faces.size();

	::write(out, header);
	::write(out, verts);
	::write(out, faces);
}

MDB_file::Hook::Hook()
{
	type = HOOK;
	memcpy(header.type, type_str(), 4);
	header.packet_size = 0;
	memset(header.name, 0, sizeof(header.name));
	header.point_type = 0;
	header.point_size = 0;
	header.position = Vector3<float>(0, 0, 0);

	for (int i = 0; i < 3; ++i)
		for (int j = 0; j < 3; ++j)
			header.orientation[i][j] = 0;
}

MDB_file::Hook::Hook(std::istream& in)
{
	read(in);
}

uint32_t MDB_file::Hook::packet_size()
{
	return sizeof(Hook_header);
}

void MDB_file::Hook::read(std::istream& in)
{
	type = HOOK;

	::read(in, header);
}

void MDB_file::Hook::write(std::ostream& out)
{
	header.packet_size = packet_size() - sizeof(Packet_header);

	::write(out, header);
}

MDB_file::Walk_mesh::Walk_mesh()
{
	type = WALK;
	memcpy(header.type, type_str(), 4);
	header.packet_size = 0;
	memset(header.name, 0, sizeof(header.name));
	header.ui_flags = 0;
	header.vertex_count = 0;
	header.face_count = 0;
}

MDB_file::Walk_mesh::Walk_mesh(std::istream& in)
{
	read(in);
}

uint32_t MDB_file::Walk_mesh::packet_size()
{
	return sizeof(Walk_mesh_header) +
	       sizeof(Walk_mesh_vertex) * verts.size() +
	       sizeof(Walk_mesh_face) * faces.size();
}

void MDB_file::Walk_mesh::read(std::istream& in)
{
	type = WALK;

	::read(in, header);

	verts.resize(header.vertex_count);
	::read(in, verts);

	faces.resize(header.face_count);
	::read(in, faces);
}

void MDB_file::Walk_mesh::write(std::ostream& out)
{
	header.packet_size = packet_size() - sizeof(Packet_header);
	header.vertex_count = verts.size();
	header.face_count = faces.size();

	::write(out, header);
	::write(out, verts);
	::write(out, faces);
}

MDB_file::Collision_spheres::Collision_spheres()
{
	type = COLS;
	memcpy(header.type, type_str(), 4);
	header.packet_size = 0;
	header.sphere_count = 0;
}

MDB_file::Collision_spheres::Collision_spheres(std::istream & in)
{
	read(in);
}

uint32_t MDB_file::Collision_spheres::packet_size()
{
	return sizeof(Collision_spheres_header) +
		sizeof(Collision_sphere) * spheres.size();
}

void MDB_file::Collision_spheres::read(std::istream & in)
{
	type = COLS;

	::read(in, header);

	spheres.resize(header.sphere_count);
	::read(in, spheres);
}

void MDB_file::Collision_spheres::write(std::ostream & out)
{
	header.packet_size = packet_size() - sizeof(Packet_header);

	::write(out, header);
	::write(out, spheres);	
}

MDB_file::Hair::Hair()
{
	type = HAIR;
	memcpy(header.type, type_str(), 4);
	header.packet_size = 0;
	memset(header.name, 0, sizeof(header.name));
	header.shortening_behavior = HSB_LOW;
	header.position = Vector3<float>(0, 0, 0);

	for (int i = 0; i < 3; ++i)
		for (int j = 0; j < 3; ++j)
			header.orientation[i][j] = 0;
}

MDB_file::Hair::Hair(std::istream & in)
{
	read(in);
}

uint32_t MDB_file::Hair::packet_size()
{
	return sizeof(Hair_header);
}

void MDB_file::Hair::read(std::istream & in)
{
	type = HAIR;

	::read(in, header);
}

void MDB_file::Hair::write(std::ostream & out)
{
	header.packet_size = packet_size() - sizeof(Packet_header);

	::write(out, header);
}

MDB_file::Helm::Helm()
{
	type = HELM;
	memcpy(header.type, type_str(), 4);
	header.packet_size = 0;
	memset(header.name, 0, sizeof(header.name));
	header.hiding_behavior = HHHB_NONE_HIDDEN;
	header.position = Vector3<float>(0, 0, 0);

	for (int i = 0; i < 3; ++i)
		for (int j = 0; j < 3; ++j)
			header.orientation[i][j] = 0;
}

MDB_file::Helm::Helm(std::istream & in)
{
	read(in);
}

uint32_t MDB_file::Helm::packet_size()
{
	return sizeof(Helm_header);
}

void MDB_file::Helm::read(std::istream & in)
{
	type = HELM;

	::read(in, header);
}

void MDB_file::Helm::write(std::ostream & out)
{
	header.packet_size = packet_size() - sizeof(Packet_header);

	::write(out, header);
}