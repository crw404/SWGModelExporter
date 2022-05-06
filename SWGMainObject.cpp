#include "stdafx.h"
#include "SWGMainObject.h"


void SWGMainObject::beginParsingProcess(std::queue<std::string> queueArray)
{
	p_queueArray = queueArray;
	
	while (!p_queueArray.empty())
	{
		std::string assetName;
		assetName = p_queueArray.front();
		p_queueArray.pop();

		// normalize path
		std::replace_if(assetName.begin(), assetName.end(), [](const char& value) { return value == '\\'; }, '/');
		std::string ext = assetName.substr(assetName.length() - 3);
		boost::to_lower(ext);

		// skip already parsed object
		if (p_Context.object_list.find(assetName) != p_Context.object_list.end())
			continue;

		std::cout << "Processing : " << assetName << std::endl;
		if (assetName == "appearance/animation/all_b_entertained.ans")
		{
			std::cout << "Asset found" << std::endl;
		}
		// do not try find object on this step
		std::vector<uint8_t> buffer;
		
		if (!p_Library->get_object(assetName, buffer))
			continue;

		//special processing for pure binary files (WAV, DDS, TGA, etc)
		if (ext == "dds")
		{
			auto texture = DDS_Texture::construct(assetName, buffer.data(), buffer.size());
			if (texture)
				p_Context.object_list.insert(std::make_pair(assetName, std::dynamic_pointer_cast<Base_object>(texture)));

			continue;
		}
		p_Buffer = IFF_file(buffer);

		
		p_Buffer.combinedObjectProcess(m_selected_parser);

		if (m_selected_parser->is_object_parsed())
		{
			auto object = m_selected_parser->get_parsed_object();
			if (object)
			{
				object->set_object_name(assetName);

				p_Context.object_list.insert(make_pair(assetName, object));

				if (ext == "mgn")
				{
					std::shared_ptr meshPtr = std::dynamic_pointer_cast<Animated_mesh>(object);
					Animated_mesh meshStore = *meshPtr;
					uint32_t level = meshStore.getLodLevel();

					switch (level)
					{
						case 0:
						{
							if (p_CompleteModels.size() == 0)
							{
								std::vector<Animated_mesh> firstOne;
								p_CompleteModels.push_back(firstOne);
							}
							p_CompleteModels.at(0).push_back(meshStore);
							break;
						}
						case 1:
						{
							if (p_CompleteModels.size() == 1)
							{
								std::vector<Animated_mesh> firstOne;
								p_CompleteModels.push_back(firstOne);
							}
							p_CompleteModels.at(1).push_back(meshStore);
							break;
						}
						case 2:
						{
							if (p_CompleteModels.size() == 2)
							{
								std::vector<Animated_mesh> firstOne;
								p_CompleteModels.push_back(firstOne);
							}
							p_CompleteModels.at(2).push_back(meshStore);
							break;
						}
						case 3:
						{
							if (p_CompleteModels.size() == 3)
							{
								std::vector<Animated_mesh> firstOne;
								p_CompleteModels.push_back(firstOne);
							}
							p_CompleteModels.at(3).push_back(meshStore);
							break;
						}
						default:
							break;
					}

					int a = 0;
					a++;
				}

				std::set<std::string> references_objects = object->get_referenced_objects();
				std::queue<std::string>& referenceArray = p_queueArray;
				Context& referenceContext = p_Context;

				std::for_each(references_objects.begin(), references_objects.end(),
					[&referenceContext, &assetName, &referenceArray](const std::string& object_name)
					{
						if (referenceContext.object_list.find(object_name) == referenceContext.object_list.end() &&
							referenceContext.unknown.find(object_name) == referenceContext.unknown.end())
						{
							referenceContext.opened_by[object_name] = assetName;
							referenceArray.push(object_name);
						}
					}
				);


			}
		}
		else
		{
			std::cout << "Objects of this type could not be converted at this time. Sorry!" << std::endl;
			p_Context.unknown.insert(p_fullName);
		}


	}

}


void SWGMainObject::CombineMeshProcess(std::shared_ptr<Animated_mesh> mainMesh, std::shared_ptr<Animated_mesh> secondaryMesh)
{
	std::shared_ptr<Animated_mesh> first = mainMesh;
	std::shared_ptr<Animated_mesh> second = secondaryMesh;

	std::vector<std::string> jointsToAdd;
	std::vector<Animated_mesh::Vertex>	verticiesToAdd;
	std::vector<Geometry::Vector3> normalsToAdd;
	std::vector<Geometry::Vector4> lightingNormalsToAdd;
	std::vector<Animated_mesh::Shader_appliance> shaderToAdd;

	for (auto jointIterator : secondaryMesh->get_joint_names())
	{
		bool jointFound = false;
		for (auto firstJointInterator : mainMesh->get_joint_names())
		{
			if (jointIterator == firstJointInterator)
			{
				jointFound = true;
				break;
			}
		}

		if (!jointFound)
		{
			jointsToAdd.push_back(jointIterator);
		}
	}

	for (auto addJoint : jointsToAdd)
	{
		mainMesh->add_joint_name(addJoint);
	}

	for (auto secondVertexIterator : secondaryMesh->get_vertices())
	{
		bool vertexFound = false;
		for (auto firstVertexInterator : mainMesh->get_vertices())
		{
			if (secondVertexIterator.isEqual(firstVertexInterator))
			{
				vertexFound = true;
				break;
			}
		}

		if (!vertexFound)
		{
			verticiesToAdd.push_back(secondVertexIterator);
		}
	}

	for (auto vertexAdd : verticiesToAdd)
	{
		mainMesh->addVertex(vertexAdd);
	}


	for (auto secondNormals : secondaryMesh->getNormals())
	{
		bool normalFound = false;
		for (auto firstNormalInterator : mainMesh->getNormals())
		{
			if (secondNormals == firstNormalInterator)
			{
				normalFound = true;
				break;
			}
		}

		if (!normalFound)
		{
			normalsToAdd.push_back(secondNormals);
		}
	}

	for (auto normalAdd : normalsToAdd)
	{
		mainMesh->add_normal(normalAdd);
	}

	for (auto lightingNormals : secondaryMesh->getNormalLighting())
	{
		bool normalFound = false;
		for (auto firstNormalInterator : mainMesh->getNormalLighting())
		{
			if (lightingNormals == firstNormalInterator)
			{
				normalFound = true;
				break;
			}
		}

		if (!normalFound)
		{
			lightingNormalsToAdd.push_back(lightingNormals);
		}
	}

	for (auto normalAdd : lightingNormalsToAdd)
	{
		mainMesh->add_lighting_normal(normalAdd);
	}

	for (auto secondaryShaders : secondaryMesh->getShaders())
	{
		bool shadersFound = false;
		for (auto firstShader : mainMesh->getShaders())
		{
			if (firstShader.get_name() == secondaryShaders.get_name())
			{
				shadersFound = true;
				break;
			}
		}

		if (!shadersFound)
		{
			shaderToAdd.push_back(secondaryShaders);
		}
	}

	for (auto shadersAdd : shaderToAdd)
	{
		mainMesh->addShader(shadersAdd);
	}

	int a = 0;
	a++;
}


void SWGMainObject::resolve_dependencies(const Context& context)
{
	// p_CompleteModels is the "master" vector that stores the data
	// This code is suppose to be iteratering through this data structure and making
	// modifications to what is inside of it
	std::shared_ptr< std::vector<Animated_mesh>> modelPointer = std::make_shared< std::vector<Animated_mesh>>(p_CompleteModels.at(0));

	for (auto& modelIterator : p_CompleteModels.at(0))
	{
		auto it = p_Context.opened_by.find(modelIterator.get_object_name());
		std::string openedByName;

		while (it != p_Context.opened_by.end())
		{
			openedByName = it->second;// Note: Stops on lmg nd not Sat
			it = p_Context.opened_by.find(openedByName);
		}
		// I want to edit the shaders that are inside the data structure.
		// There are multiple shaders for each model. So we also need to iterate through them
		auto shaderPointer = modelIterator.ShaderPointer();// Just getting the pointer that is pointing to the vector containing the shaders used
		
		for(auto& shaderIterator : modelIterator.getShaders())
		{
			auto obj_it = p_Context.object_list.find(shaderIterator.get_name());
			if (obj_it != p_Context.object_list.end())
			{
				if (std::dynamic_pointer_cast<Shader>(obj_it->second))
				{
					// Once checks are passed, we need to set the shader definition.
					// This is a where everything gets tricky.
					// Function takes in a shared pointer and sets
					// that equal to the definition object stored within the shader
					// However, when viewing from the scope of the p_CompleteModels,
					// the definition object is still empty
					shaderIterator.set_definition(std::dynamic_pointer_cast<Shader>(obj_it->second));
				}
			}
		}
	

		auto bad_shaders = std::any_of(modelIterator.getShaders().begin(), modelIterator.getShaders().end(),
			[](const Animated_mesh::Shader_appliance& shader) { return shader.get_definition() == nullptr; });

		if (bad_shaders)
		{
			std::vector<uint8_t> counters(modelIterator.get_vertices().size(), 0);
			for (auto& shader : modelIterator.getShaders())
			{
				for (auto& vert_idx : shader.get_pos_indexes())
				{
					if (vert_idx >= counters.size())
						break;
					counters[vert_idx]++;
				}


				if (shader.get_definition() == nullptr)
				{
					for (auto& vert_idx : shader.get_pos_indexes())
					{
						if (vert_idx >= counters.size())
							break;
						counters[vert_idx]--;
					}
				}
			}

			auto safe_clear = is_partitioned(counters.begin(), counters.end(),
				[](uint8_t val) { return val > 0; });

			if (safe_clear)
			{
				// we can do safe clear of trouble shader, if not - oops
				auto beg = find_if(counters.begin(), counters.end(),
					[](uint8_t val) { return val == 0; });
				auto idx = distance(counters.begin(), beg);
				std::vector<Animated_mesh::Vertex> vertexMesh = modelIterator.get_vertices();

				vertexMesh.erase(vertexMesh.begin() + idx, vertexMesh.end());
				
				for (size_t idx = 0; idx < modelIterator.get_vertices().size(); ++idx)
				{
					if (modelIterator.getShaders().at(idx).get_definition() == nullptr)
					{
						modelIterator.getShaders().erase(modelIterator.getShaders().begin() + idx);
						break;
					}
				}
			}
		}

		auto mesh_description = std::dynamic_pointer_cast<Animated_object_descriptor>(p_Context.object_list.find(openedByName)->second);

		// get skeletons
		size_t skel_idx = 0;
		for (auto it_skel = modelIterator.getSkeletonNames().begin(); it_skel != modelIterator.getSkeletonNames().end(); ++it_skel, ++skel_idx)
		{
			auto skel_name = *it_skel;
			// check name against name in description
			if (mesh_description)
			{
				auto descr_skel_name = mesh_description->get_skeleton_name(skel_idx);
				if (!boost::iequals(skel_name, descr_skel_name))
					skel_name = descr_skel_name;
			}
			auto obj_it = p_Context.object_list.find(skel_name);
			if (obj_it != p_Context.object_list.end() && std::dynamic_pointer_cast<Skeleton>(obj_it->second))
				modelIterator.getSkeleton().emplace_back(skel_name, std::dynamic_pointer_cast<Skeleton>(obj_it->second)->clone());
		}

		if (!openedByName.empty() && modelIterator.getSkeleton().size() > 1 && mesh_description)
		{
			// check if we opened through SAT and have multiple skeleton definitions
			//  then we have unite them to one, so we need SAT to get join information from it's skeleton info;
			auto skel_count = mesh_description->get_skeletons_count();
			std::shared_ptr<Skeleton> root_skeleton;
			for (uint32_t skel_idx = 0; skel_idx < skel_count; ++skel_idx)
			{
				auto skel_name = mesh_description->get_skeleton_name(skel_idx);
				auto point_name = mesh_description->get_skeleton_attach_point(skel_idx);

				auto this_it = std::find_if(modelIterator.getSkeleton().begin(), modelIterator.getSkeleton().end(),
					[&skel_name](const std::pair<std::string, std::shared_ptr<Skeleton>>& info)
					{
						return (boost::iequals(skel_name, info.first));
					});

				if (point_name.empty() && this_it != modelIterator.getSkeleton().end())
					// mark this skeleton as root
					root_skeleton = this_it->second;
				else if (this_it != modelIterator.getSkeleton().end())
				{
					// attach skeleton to root
					root_skeleton->join_skeleton_to_point(point_name, this_it->second);
					// remove it from used list
					modelIterator.getSkeleton().erase(this_it);
				}
			}
		}
	}
}



void SWGMainObject::store(const std::string& path, const Context& context)
{

	// extract object name and make full path name
	boost::filesystem::path obj_name(p_CompleteModels.at(0).at(0).get_object_name());
	boost::filesystem::path target_path(path);
	target_path /= obj_name.filename();

	target_path.replace_extension("fbx");

	// check directory existance
	auto directory = target_path.parent_path();
	if (!boost::filesystem::exists(directory))
		boost::filesystem::create_directories(directory);

	if (boost::filesystem::exists(target_path))
		boost::filesystem::remove(target_path);

	// get lod level (by _lX end of file name). If there is no such pattern - lod level will be zero.
	int lodLevel = p_CompleteModels.at(0).at(0).getLodLevel();
	obj_name = obj_name.filename();
	obj_name.replace_extension();
	std::string name = obj_name.string();
	std::string mainObjectName = p_CompleteModels.at(0).at(0).get_object_name();

	// init FBX manager
	FbxManager* fbx_manager_ptr = FbxManager::Create();
	if (fbx_manager_ptr == nullptr)
		return;

	FbxIOSettings* ios_ptr = FbxIOSettings::Create(fbx_manager_ptr, IOSROOT);
	ios_ptr->SetIntProp(EXP_FBX_EXPORT_FILE_VERSION, FBX_FILE_VERSION_7400);
	fbx_manager_ptr->SetIOSettings(ios_ptr);

	FbxExporter* exporter_ptr = FbxExporter::Create(fbx_manager_ptr, "");
	if (!exporter_ptr)
		return;

	exporter_ptr->SetFileExportVersion(FBX_2014_00_COMPATIBLE);
	bool result = exporter_ptr->Initialize(target_path.string().c_str(), -1, fbx_manager_ptr->GetIOSettings());
	if (!result)
	{
		auto status = exporter_ptr->GetStatus();
		std::cout << "FBX error: " << status.GetErrorString() << std::endl;
		return;
	}
	FbxScene* scene_ptr = FbxScene::Create(fbx_manager_ptr, mainObjectName.c_str());
	if (!scene_ptr)
		return;

	scene_ptr->GetGlobalSettings().SetSystemUnit(FbxSystemUnit::m);
	auto scale_factor = scene_ptr->GetGlobalSettings().GetSystemUnit().GetScaleFactor();
	FbxNode* mesh_node_ptr = FbxNode::Create(scene_ptr, "mesh_node"); //skeleton root?
	FbxMesh* mesh_ptr = FbxMesh::Create(scene_ptr, "mesh");

	mesh_node_ptr->SetNodeAttribute(mesh_ptr);
	scene_ptr->GetRootNode()->AddChild(mesh_node_ptr);

	// prepare vertices
	uint32_t vertices_num = 0;
	uint32_t normalsNum = 0;
	uint32_t counter = 0;

	for (int cc = 0; cc < p_CompleteModels.at(0).size(); cc++)
	{
		auto& modelIterator = p_CompleteModels.at(0).at(cc);
		auto temp = static_cast<uint32_t>(modelIterator.get_vertices().size());
		vertices_num += static_cast<uint32_t>(modelIterator.get_vertices().size());
		normalsNum += static_cast<uint32_t>(modelIterator.getNormals().size());
	}

	mesh_ptr->SetControlPointCount(vertices_num);
	auto mesh_vertices = mesh_ptr->GetControlPoints();


	//for (auto& modelIterator : p_CompleteModels.at(0))
	for (int cc = 0; cc < p_CompleteModels.at(0).size(); cc++)
	{
		auto& modelIterator = p_CompleteModels.at(0).at(cc);
		for (uint32_t vertexCounter = 0; vertexCounter < modelIterator.get_vertices().size(); vertexCounter++)
		{
			const auto& pt = modelIterator.get_vertices()[vertexCounter].get_position();
			mesh_vertices[counter + vertexCounter] = FbxVector4(pt.x, pt.y, pt.z);
		}
		counter += modelIterator.get_vertices().size();
	}

	// add material layer
	auto material_layer = mesh_ptr->CreateElementMaterial();
	material_layer->SetMappingMode(FbxLayerElement::eByPolygon);
	material_layer->SetReferenceMode(FbxLayerElement::eIndexToDirect);

	// process polygons
	std::vector<uint32_t> normal_indexes;
	std::vector<uint32_t> tangents_idxs;
	std::vector<Graphics::Tex_coord> uvs;
	std::vector<uint32_t> uv_indexes;
	counter = 0;
	uint32_t normalCounter = 0;
	for (int cc = 0; cc < p_CompleteModels.at(0).size(); cc++)
	{
		auto& modelIterator = p_CompleteModels.at(0).at(cc);
		uint32_t shaderCounter = 0;
		for (uint32_t shader_idx = 0; shader_idx < modelIterator.getShaders().size(); ++shader_idx)
		{
			auto& shader = modelIterator.getShaders().at(shader_idx);
			if (shader.get_definition())
			{
				auto material_ptr = FbxSurfacePhong::Create(scene_ptr, shader.get_name().c_str());
				material_ptr->ShadingModel.Set("Phong");

				auto& material = shader.get_definition()->material();
				auto& textures = shader.get_definition()->textures();

				// create material for this shader
				material_ptr->Ambient.Set(FbxDouble3(material.ambient.r, material.ambient.g, material.ambient.g));
				material_ptr->Diffuse.Set(FbxDouble3(material.diffuse.r, material.diffuse.g, material.diffuse.g));
				material_ptr->Emissive.Set(FbxDouble3(material.emissive.r, material.emissive.g, material.emissive.g));
				material_ptr->Specular.Set(FbxDouble3(material.specular.r, material.specular.g, material.specular.g));

				// add texture definitions
				for (auto& texture_def : textures)
				{
					FbxFileTexture* texture = FbxFileTexture::Create(scene_ptr, texture_def.tex_file_name.c_str());
					boost::filesystem::path tex_path(path);
					tex_path /= texture_def.tex_file_name;
					tex_path.replace_extension("tga");

					texture->SetFileName(tex_path.string().c_str());
					texture->SetTextureUse(FbxTexture::eStandard);
					texture->SetMaterialUse(FbxFileTexture::eModelMaterial);
					texture->SetMappingType(FbxTexture::eUV);
					texture->SetWrapMode(FbxTexture::eRepeat, FbxTexture::eRepeat);
					texture->SetTranslation(0.0, 0.0);
					texture->SetScale(1.0, 1.0);
					texture->SetTranslation(0.0, 0.0);
					switch (texture_def.texture_type)
					{
					case Shader::texture_type::main:
						material_ptr->Diffuse.ConnectSrcObject(texture);
						break;
					case Shader::texture_type::normal:
						material_ptr->Bump.ConnectSrcObject(texture);
						break;
					case Shader::texture_type::specular:
						material_ptr->Specular.ConnectSrcObject(texture);
						break;
					}
				}

				mesh_ptr->GetNode()->AddMaterial(material_ptr);

				// get geometry element
				auto& triangles = shader.get_triangles();
				auto& positions = shader.get_pos_indexes();
				auto& normals = shader.get_normal_indexes();
				auto& tangents = shader.get_light_indexes();

				auto idx_offset = static_cast<uint32_t>(uvs.size());
				copy(shader.get_texels().begin(), shader.get_texels().end(), back_inserter(uvs));

				normal_indexes.reserve(normal_indexes.size() + normals.size());
				tangents_idxs.reserve(tangents_idxs.size() + tangents.size());

				for (uint32_t tri_idx = 0; tri_idx < triangles.size(); ++tri_idx)
				{
					auto& tri = triangles[tri_idx];
					mesh_ptr->BeginPolygon(shader_idx + shaderCounter, -1, shader_idx + shaderCounter, false);
					for (size_t i = 0; i < 3; ++i)
					{
						auto remapped_pos_idx = positions[tri.points[i]] + counter;
						mesh_ptr->AddPolygon(remapped_pos_idx);

						auto remapped_normal_idx = normals[tri.points[i]] + normalCounter;
						normal_indexes.emplace_back(remapped_normal_idx);

						if (!tangents.empty())
						{
							auto remapped_tangent = tangents[tri.points[i]];
							tangents_idxs.emplace_back(remapped_tangent);
						}
						uv_indexes.emplace_back(idx_offset + tri.points[i]);
					}
					mesh_ptr->EndPolygon();
				}
			}
		}
		shaderCounter += modelIterator.getShaders().size();
		counter += static_cast<uint32_t>(modelIterator.get_vertices().size());
		normalCounter += static_cast<uint32_t>(modelIterator.getNormals().size());
	}
	counter = 0;
	// add UVs
	FbxGeometryElementUV* uv_ptr = mesh_ptr->CreateElementUV("UVSet1");
	uv_ptr->SetMappingMode(FbxGeometryElement::eByPolygonVertex);
	uv_ptr->SetReferenceMode(FbxGeometryElement::eIndexToDirect);

	std::for_each(uvs.begin(), uvs.end(), [&uv_ptr](const Graphics::Tex_coord& coord)
		{
			uv_ptr->GetDirectArray().Add(FbxVector2(coord.u, coord.v));
		});
	std::for_each(uv_indexes.begin(), uv_indexes.end(), [&uv_ptr](const uint32_t idx)
		{
			uv_ptr->GetIndexArray().Add(idx);
		});

	// add normals
	if (!normal_indexes.empty())
	{
		FbxGeometryElementNormal* normals_ptr = mesh_ptr->CreateElementNormal();
		normals_ptr->SetMappingMode(FbxGeometryElementNormal::eByPolygonVertex);
		normals_ptr->SetReferenceMode(FbxGeometryElement::eIndexToDirect);
		auto& direct_array = normals_ptr->GetDirectArray();

		for (auto& modelIterator : p_CompleteModels.at(0))
		{
			std::for_each(modelIterator.getNormals().begin(), modelIterator.getNormals().end(),
				[&direct_array](const Geometry::Vector3& elem)
				{
					direct_array.Add(FbxVector4(elem.x, elem.y, elem.z)); // TODO: Check this
				});
		}

		auto& index_array = normals_ptr->GetIndexArray();
		std::for_each(normal_indexes.begin(), normal_indexes.end(),
			[&index_array](const uint32_t& idx) { index_array.Add(idx); });
	}

	// add tangents
	if (!tangents_idxs.empty())
	{
		FbxGeometryElementTangent* normals_ptr = mesh_ptr->CreateElementTangent();
		normals_ptr->SetMappingMode(FbxGeometryElement::eByPolygonVertex);
		normals_ptr->SetReferenceMode(FbxGeometryElement::eIndexToDirect);
		auto& direct_array = normals_ptr->GetDirectArray();

		for (auto& modelIterator : p_CompleteModels.at(0))
		{
			std::for_each(modelIterator.getNormalLighting().begin(), modelIterator.getNormalLighting().end(),
				[&direct_array](const Geometry::Vector4& elem)
				{
					direct_array.Add(FbxVector4(elem.x, elem.y, elem.z)); // TODO: Check this
				});
		}

		auto& index_array = normals_ptr->GetIndexArray();
		std::for_each(tangents_idxs.begin(), tangents_idxs.end(),
			[&index_array](const uint32_t& idx) { index_array.Add(idx); });
	}

	mesh_ptr->BuildMeshEdgeArray();
	std::vector<Skeleton::Bone> BoneInfoList;
	// build skeletons
	for (int cc = 0; cc < p_CompleteModels.at(0).size(); cc++)
	{
		auto& modelIterator = p_CompleteModels.at(0).at(cc);

		uint32_t m_lod_level = modelIterator.getLodLevel();

		std::for_each(modelIterator.getSkeleton().begin(), modelIterator.getSkeleton().end(),
			[&scene_ptr, &mesh_node_ptr, &BoneInfoList, &m_lod_level, this](const std::pair<std::string, std::shared_ptr<Skeleton>>& item)
			{
				if (item.second->get_lod_count() > m_lod_level)
				{
					//item.second->set_current_lod(m_lod_level);
					// Might need to check for duplicates here
					for (auto& boneIterator : item.second->getBonesatLOD(0))
					{
						bool foundBone = false;
						if (m_bones.size() == m_lod_level)
						{
							std::vector<Skeleton::Bone> firstOne;
							m_bones.push_back(firstOne);
						}

						for (auto otherBoneIterator : m_bones.at(m_lod_level))
						{
							if (otherBoneIterator.name == boneIterator.name)
							{
								foundBone = true;
								break;
							}
						}

						if (!foundBone)
							m_bones.at(m_lod_level).push_back(boneIterator);
					}

				}
			});
	}

	BoneInfoList = generateSkeletonInScene(scene_ptr, mesh_node_ptr);



	// build morph targets
	// prepare base vector
	FbxBlendShape* blend_shape_ptr = FbxBlendShape::Create(scene_ptr, "BlendShapes");
	for (int cc = 0; cc < p_CompleteModels.at(0).size(); cc++)
	{
		auto& modelIterator = p_CompleteModels.at(0).at(cc);

		auto total_vertices = modelIterator.get_vertices().size();
		for (const auto& morph : modelIterator.getMorphs())
		{
			FbxBlendShapeChannel* morph_channel = FbxBlendShapeChannel::Create(scene_ptr, morph.get_name().c_str());
			FbxShape* shape = FbxShape::Create(scene_ptr, morph_channel->GetName());

			shape->InitControlPoints(static_cast<int>(total_vertices));
			auto shape_vertices = shape->GetControlPoints();

			// copy base vertices to shape vertices
			for (size_t idx = 0; idx < total_vertices; ++idx)
			{
				auto& pos = modelIterator.get_vertices().at(idx).get_position();
				shape_vertices[idx].Set(pos.x, pos.y, pos.z);
			}

			// apply morph
			for (auto& morph_pt : morph.get_positions())
			{
				size_t idx = morph_pt.first;
				auto& offset = morph_pt.second;
				if (idx >= total_vertices)
					continue;
				auto& pos = modelIterator.get_vertices().at(idx).get_position();
				shape_vertices[idx].Set(pos.x + offset.x, pos.y + offset.y, pos.z + offset.z);
			}

			if (!normal_indexes.empty() && !p_Context.batch_mode)
			{
				// get normals
				auto normal_element = shape->CreateElementNormal();
				normal_element->SetMappingMode(FbxGeometryElement::eByPolygonVertex);
				normal_element->SetReferenceMode(FbxGeometryElement::eIndexToDirect);

				auto& direct_array = normal_element->GetDirectArray();
				// set a base normals
				std::for_each(modelIterator.getNormals().begin(), modelIterator.getNormals().end(),
					[&direct_array](const Geometry::Vector3& elem)
					{
						direct_array.Add(FbxVector4(elem.x, elem.y, elem.z));
					});


				for (auto& morph_normal : morph.get_normals())
				{
					uint32_t idx = morph_normal.first;
					auto& offset = morph_normal.second;
					auto& base = modelIterator.getNormals().at(idx);
					direct_array[idx].Set(base.x + offset.x, base.y + offset.y, base.z + offset.z);
				}

				auto& index_array = normal_element->GetIndexArray();
				std::for_each(normal_indexes.begin(), normal_indexes.end(),
					[&index_array](const uint32_t& idx) { index_array.Add(idx); });
			}

			if (!tangents_idxs.empty() && !p_Context.batch_mode)
			{
				// get tangents
				auto tangents_ptr = shape->CreateElementTangent();
				tangents_ptr->SetMappingMode(FbxGeometryElement::eByPolygonVertex);
				tangents_ptr->SetReferenceMode(FbxGeometryElement::eIndexToDirect);

				auto& direct_array = tangents_ptr->GetDirectArray();
				std::for_each(modelIterator.getNormalLighting().begin(), modelIterator.getNormalLighting().end(),
					[&direct_array](const Geometry::Vector4& elem)
					{
						direct_array.Add(FbxVector4(elem.x, elem.y, elem.z));
					});

				for (auto& morph_tangent : morph.get_tangents())
				{
					uint32_t idx = morph_tangent.first;
					auto& offset = morph_tangent.second;
					auto& base = modelIterator.getNormalLighting().at(idx);
					direct_array[idx].Set(base.x + offset.x, base.y + offset.y, base.z + offset.z);
				}

				auto& index_array = tangents_ptr->GetIndexArray();
				std::for_each(tangents_idxs.begin(), tangents_idxs.end(),
					[&index_array](const uint32_t& idx) { index_array.Add(idx); });
			}

			auto success = morph_channel->AddTargetShape(shape);
			success = blend_shape_ptr->AddBlendShapeChannel(morph_channel);
		}
	}
	mesh_node_ptr->GetGeometry()->AddDeformer(blend_shape_ptr);

	// Build Animations (Note: Not sure if this is the best place to put them. But should be after the skeletons
	std::vector<std::shared_ptr<Animation>> animationList;

	// First sort out the animation objects first
	for (auto baseObjectiterator : p_Context.object_list)
	{
		if (baseObjectiterator.second->get_object_name().find("ans") != std::string::npos)
		{
			std::shared_ptr<Animation> inputObject = std::dynamic_pointer_cast<Animation>(baseObjectiterator.second);
			animationList.push_back(inputObject);
		}
	}

	QuatExpand::UncompressQuaternion decompressValues;
	decompressValues.install();

	// Next loop through the entire animation list
	//for (auto animationObject : animationList)
	//{
	for (int i = 0; i < animationList.size(); i++)// This method is esy for debugging
	{
		auto animationObject = animationList.at(i);
		if (animationObject)
		{
			std::string stackName = animationObject->get_object_name();
			std::string firstErase = "appearance/animation/";
			std::string secondErase = ".ans";

			size_t pos = stackName.find(firstErase);
			if (pos != std::string::npos)
				stackName.erase(pos, firstErase.length());

			size_t pos2 = stackName.find(secondErase);
			if (pos2 != std::string::npos)
				stackName.erase(pos2, secondErase.length());

			FbxString animationStackName = FbxString(stackName.c_str());
			fbxsdk::FbxAnimStack* animationStack = fbxsdk::FbxAnimStack::Create(scene_ptr, animationStackName);

			FbxAnimLayer* animationLayer = FbxAnimLayer::Create(scene_ptr, "Base Layer");
			animationStack->AddMember(animationLayer);

			FbxGlobalSettings& sceneGlobaleSettings = scene_ptr->GetGlobalSettings();
			double currentFrameRate = FbxTime::GetFrameRate(sceneGlobaleSettings.GetTimeMode());
			if (animationObject->get_info().FPS != currentFrameRate)
			{
				FbxTime::EMode computeTimeMode = FbxTime::ConvertFrameRateToTimeMode(animationObject->get_info().FPS);
				FbxTime::SetGlobalTimeMode(computeTimeMode, computeTimeMode == FbxTime::eCustom ? animationObject->get_info().FPS : 0.0);
				sceneGlobaleSettings.SetTimeMode(computeTimeMode);
				if (computeTimeMode == FbxTime::eCustom)
				{
					sceneGlobaleSettings.SetCustomFrameRate(animationObject->get_info().FPS);
				}
			}

			FbxTime exportedStartTime, exportedStopTime;
			exportedStartTime.SetSecondDouble(0.0f);
			exportedStopTime.SetSecondDouble((double)animationObject->get_info().frame_count / animationObject->get_info().FPS);

			FbxTimeSpan exportedTimeSpan;
			exportedTimeSpan.Set(exportedStartTime, exportedStopTime);
			animationStack->SetLocalTimeSpan(exportedTimeSpan);

			for (auto& boneIterator : animationObject->get_bones())
			{
				std::vector<FbxNode*> treeBranch;
				treeBranch.push_back(mesh_node_ptr->GetChild(0));// The first node to start with is the child of the root node
				std::string boneName = boneIterator.name;

				for (int i = 0; i < treeBranch.size(); i++)// Loop through the branches of the tree
				{
					std::string treeBranchName = treeBranch.at(i)->GetName();// Grab the name of the current element Also for debugging

					boost::to_lower(treeBranchName);
					boost::to_lower(boneName);

					if (treeBranchName == boneName)// Found a match between the selected 
					{
						Skeleton::Bone skeletonBone = Skeleton::Bone("test");
						for (auto& boneInfoIterator : BoneInfoList)
						{
							std::string searchBoneName = boneInfoIterator.name;
							boost::to_lower(searchBoneName);

							if (searchBoneName == boneName)
							{
								skeletonBone = boneInfoIterator;
								break;
							}
						}

						// For easy access placing pointers to the bones
						FbxNode* rootSkeleton = mesh_node_ptr;
						FbxNode* boneToUse = treeBranch.at(i);
						fbxsdk::FbxAMatrix& globalNode = boneToUse->EvaluateLocalTransform();
						// The bone iteratator will have all the animation info for the specific bone
						fbxsdk::FbxAnimCurve* Curves[9];

						Curves[0] = boneToUse->LclTranslation.GetCurve(animationLayer, FBXSDK_CURVENODE_COMPONENT_X, true);
						Curves[1] = boneToUse->LclTranslation.GetCurve(animationLayer, FBXSDK_CURVENODE_COMPONENT_Y, true);
						Curves[2] = boneToUse->LclTranslation.GetCurve(animationLayer, FBXSDK_CURVENODE_COMPONENT_Z, true);

						Curves[3] = boneToUse->LclRotation.GetCurve(animationLayer, FBXSDK_CURVENODE_COMPONENT_X, true);
						Curves[4] = boneToUse->LclRotation.GetCurve(animationLayer, FBXSDK_CURVENODE_COMPONENT_Y, true);
						Curves[5] = boneToUse->LclRotation.GetCurve(animationLayer, FBXSDK_CURVENODE_COMPONENT_Z, true);

						Curves[6] = boneToUse->LclScaling.GetCurve(animationLayer, FBXSDK_CURVENODE_COMPONENT_X, true);
						Curves[7] = boneToUse->LclScaling.GetCurve(animationLayer, FBXSDK_CURVENODE_COMPONENT_Y, true);
						Curves[8] = boneToUse->LclScaling.GetCurve(animationLayer, FBXSDK_CURVENODE_COMPONENT_Z, true);


						for (fbxsdk::FbxAnimCurve* Curve : Curves)
						{
							Curve->KeyModifyBegin();
						}

						for (int frameCounter = 0; frameCounter < animationObject->get_info().frame_count + 1; frameCounter++)
						{
							// For each frame, we need to build the translation vector and the rotation vector
							FbxVector4 TranslationVector;
							FbxVector4 RotationVector;

							if (boneIterator.hasXAnimatedTranslation)
							{

								std::vector<float> translationValues = animationObject->getCHNLValues().at(boneIterator.x_translation_channel_index);

								if (translationValues.size() != animationObject->get_info().frame_count + 1)
								{
									std::cout << "Mis-match size";
								}

								float translationValue = translationValues.at(frameCounter);
								if (translationValue > -1000.0)
								{
									TranslationVector.mData[0] = translationValue;
								}
								else
								{
									TranslationVector.mData[0] = -1000.0;
								}
							}
							else
							{
								float translationValue = animationObject->getStaticTranslationValues().at(boneIterator.x_translation_channel_index);
								TranslationVector.mData[0] = translationValue;
							}

							if (boneIterator.hasYAnimatedTranslation)
							{
								std::vector<float> translationValues = animationObject->getCHNLValues().at(boneIterator.y_translation_channel_index);

								if (translationValues.size() != animationObject->get_info().frame_count + 1)
								{
									std::cout << "Mis-match size";
								}

								float translationValue = translationValues.at(frameCounter);
								if (translationValue > -1000.0)
								{
									TranslationVector.mData[1] = translationValue;
								}
								else
								{
									//Need to do something here....
									//cout << "Frame Skipped";
									TranslationVector.mData[1] = -1000.0;
								}
							}
							else
							{
								float translationValue = animationObject->getStaticTranslationValues().at(boneIterator.y_translation_channel_index);
								TranslationVector.mData[1] = translationValue;
							}

							if (boneIterator.hasZAnimatedTranslation)
							{
								std::vector<float> translationValues = animationObject->getCHNLValues().at(boneIterator.z_translation_channel_index);

								if (translationValues.size() != animationObject->get_info().frame_count + 1)
								{
									std::cout << "Mis-match size";
								}

								float translationValue = translationValues.at(frameCounter);
								if (translationValue > -1000.0)
								{
									TranslationVector.mData[2] = translationValue;
								}
								else
								{
									//Need to do something here....
									//cout << "Frame Skipped";
									TranslationVector.mData[2] = -1000.0;
								}
							}
							else
							{
								float translationValue = animationObject->getStaticTranslationValues().at(boneIterator.z_translation_channel_index);
								TranslationVector.mData[2] = translationValue;
							}

							bool isStaticRotation = false;
							if (boneIterator.has_rotations)
							{
								std::vector<uint32_t> compressedValues = animationObject->getQCHNValues().at(boneIterator.rotation_channel_index);
								std::vector<uint8_t> FormatValues;

								if (compressedValues.size() - 1 != animationObject->get_info().frame_count + 1)
								{
									std::cout << "Mis-match size";
								}

								uint32_t formatValue = compressedValues.at(0);
								uint32_t compressedValue = compressedValues.at(frameCounter + 1);

								FormatValues.push_back((formatValue & (((1 << 8) - 1) << 16)) >> 16);
								FormatValues.push_back((formatValue & (((1 << 8) - 1) << 8)) >> 8);
								FormatValues.push_back((formatValue & (((1 << 8) - 1))));

								if (compressedValue != 100)
								{
									Geometry::Vector4 Quat = decompressValues.ExpandCompressedValue(compressedValue, FormatValues[0], FormatValues[1], FormatValues[2]);

									EulerAngles result = ConvertCombineCompressQuat(Quat, skeletonBone);

									RotationVector = FbxVector4(result.roll, result.pitch, result.yaw);
								}
								else
								{
									// DO something else here
									//cout << "Frame Skipped";
									RotationVector = FbxVector4(-1000.0, -1000.0, -1000.0);
								}
							}
							else
							{
								uint32_t staticValue = animationObject->getStaticRotationValues().at(boneIterator.rotation_channel_index);
								std::vector<uint8_t> formatValues = animationObject->getStaticROTFormats().at(boneIterator.rotation_channel_index);

								Geometry::Vector4 Quat = decompressValues.ExpandCompressedValue(staticValue, formatValues[0], formatValues[1], formatValues[2]);

								EulerAngles result = ConvertCombineCompressQuat(Quat, skeletonBone, true);

								RotationVector = FbxVector4(result.roll, result.pitch, result.yaw);
								isStaticRotation = true;
							}
							FbxVector4 ScalingVector(1.0, 1.0, 1.0);

							FbxVector4 Vectors[3] = { TranslationVector, RotationVector, ScalingVector };

							double timeValue = (double)frameCounter * ((double)1.0 / (double)animationObject->get_info().FPS);
							FbxVector4 finalVector[3] = { globalNode.GetT() + TranslationVector , RotationVector, ScalingVector };

							for (int curveIndex = 0; curveIndex < 2; curveIndex++)
							{
								for (int coordinateIndex = 0; coordinateIndex < 3; coordinateIndex++)
								{
									if (Vectors[curveIndex][coordinateIndex] != -1000.0)// If the coordinate value is -1000 then this means we need to skip it since it is not a key frame
									{
										int offsetCurveIndex = (curveIndex * 3) + coordinateIndex;
										FbxTime setTime;
										setTime.SetSecondDouble(timeValue);
										uint32_t keyIndex = Curves[offsetCurveIndex]->KeyAdd(setTime);
										Curves[offsetCurveIndex]->KeySet(keyIndex, setTime, finalVector[curveIndex][coordinateIndex], frameCounter == animationObject->get_info().frame_count ? FbxAnimCurveDef::eInterpolationConstant : FbxAnimCurveDef::eInterpolationCubic);
										if (frameCounter == animationObject->get_info().frame_count)
										{
											Curves[offsetCurveIndex]->KeySetConstantMode(keyIndex, FbxAnimCurveDef::eConstantStandard);
										}
									}
									else
									{
										//cout << "Frame Skipped" << endl;
									}
								}
							}
						}

						for (fbxsdk::FbxAnimCurve* Curve : Curves)
						{
							Curve->KeyModifyEnd();
						}
					}
					else
					{
						// If not found, then we will add the children to the treeBranch vector so that we can 
						// check the children also
						if (treeBranch.at(i)->GetChildCount() > 0)
						{
							for (int ii = 0; ii < treeBranch.at(i)->GetChildCount(); ii++)
							{
								treeBranch.push_back(treeBranch.at(i)->GetChild(ii));
							}
						}
					}
				}
			}
		}
	}

	FbxSystemUnit::cm.ConvertScene(scene_ptr);

	exporter_ptr->Export(scene_ptr);
	// cleanup
	fbx_manager_ptr->Destroy();
}

SWGMainObject::EulerAngles  SWGMainObject::ConvertCombineCompressQuat(Geometry::Vector4 DecompressedQuaterion, Skeleton::Bone BoneReference, bool isStatic)
{
	const double pi = 3.14159265358979323846;
	double rotationFactor = 180.0 / pi;

	Geometry::Vector4 Quat;
	EulerAngles angles;

	FbxQuaternion AnimationQuat = FbxQuaternion(DecompressedQuaterion.x, DecompressedQuaterion.y, DecompressedQuaterion.z, DecompressedQuaterion.a);
	FbxQuaternion bind_rot_quat{ BoneReference.bind_pose_rotation.x, BoneReference.bind_pose_rotation.y, BoneReference.bind_pose_rotation.z, BoneReference.bind_pose_rotation.a };
	FbxQuaternion pre_rot_quat{ BoneReference.pre_rot_quaternion.x, BoneReference.pre_rot_quaternion.y, BoneReference.pre_rot_quaternion.z, BoneReference.pre_rot_quaternion.a };
	FbxQuaternion post_rot_quat{ BoneReference.post_rot_quaternion.x, BoneReference.post_rot_quaternion.y, BoneReference.post_rot_quaternion.z, BoneReference.post_rot_quaternion.a };

	auto full_rot = post_rot_quat * (AnimationQuat * bind_rot_quat) * pre_rot_quat;
	Quat = Geometry::Vector4(full_rot.mData[0], full_rot.mData[1], full_rot.mData[2], full_rot.mData[3]);

	double test = Quat.x * Quat.z - Quat.y * Quat.a;
	double sqx = Quat.x * Quat.x;
	double sqy = Quat.y * Quat.y;
	double sqz = Quat.z * Quat.z;
	double sqa = Quat.a * Quat.a;
	double unit = sqx + sqy + sqz + sqa;

	angles.yaw = std::atan2(2.0 * (Quat.x * Quat.y + Quat.z * Quat.a), sqx - sqy - sqz + sqa); // heading
	angles.pitch = std::asin(-2.0 * test / unit); // attitude
	angles.roll = std::atan2(2.0 * (Quat.y * Quat.z + Quat.x * Quat.a), -sqx - sqy + sqz + sqa); // bank
	
	/*double test = Quat.x * Quat.y + Quat.z * Quat.a;
	if (test < 0.499)
	{
		angles.yaw = 2.0 * std::atan2(Quat.x, Quat.a);
		angles.pitch = pi / 2.0;
		angles.roll = 0;
	}
	else if (test < -0.499)
	{
		angles.yaw = -2.0 * std::atan2(Quat.x, Quat.a);
		angles.pitch = -pi / 2.0;
		angles.roll = 0;
	}
	else
	{
		double sqx = Quat.x * Quat.x;
		double sqy = Quat.y * Quat.y;
		double sqz = Quat.z * Quat.z;

		angles.yaw = std::atan2(2.0 * Quat.y * Quat.a - 2.0 * Quat.x * Quat.z, 1.0 - 2.0 * sqy - 2.0 * sqz);
		angles.pitch = std::asin(2.0 * test);
		angles.roll = std::atan2(2.0 * Quat.x * Quat.a - 2.0 * Quat.y * Quat.z, 1.0 - 2.0 * sqx - 2.0 * sqz);
	}*/


	angles.roll *= rotationFactor;
	angles.pitch *= rotationFactor;
	angles.yaw *= rotationFactor;

	return angles;
}


std::vector<Skeleton::Bone> SWGMainObject::generateSkeletonInScene(FbxScene* scene_ptr, FbxNode* parent_ptr)
{
	assert(parent_ptr != nullptr && scene_ptr != nullptr);
	std::vector<Skeleton::Bone> boneListing;

	uint32_t boneCount = get_bones_count(0); // Turn this into a loop??

	std::vector<FbxNode*> nodes(boneCount, nullptr);
	std::vector<FbxCluster*> clusters(boneCount, nullptr);

	auto pose_ptr2 = FbxPose::Create(scene_ptr, "Rest Pose"); // Also create the binding pose
	pose_ptr2->SetIsBindPose(true);

	for (uint32_t boneCounter = 0; boneCounter < boneCount; boneCounter++)
	{
		Skeleton::Bone& bone = getBone(boneCounter, 0);

		FbxNode* node_ptr = FbxNode::Create(scene_ptr, bone.name.c_str());
		FbxSkeleton* skeleton_ptr = FbxSkeleton::Create(scene_ptr, bone.name.c_str());

		if (bone.parent_idx == -1)
			skeleton_ptr->SetSkeletonType(FbxSkeleton::eRoot);
		else
			skeleton_ptr->SetSkeletonType(FbxSkeleton::eLimbNode);

		node_ptr->SetNodeAttribute(skeleton_ptr);
		FbxQuaternion pre_rot_quat{ bone.pre_rot_quaternion.x, bone.pre_rot_quaternion.y, bone.pre_rot_quaternion.z, bone.pre_rot_quaternion.a };
		FbxQuaternion post_rot_quat{ bone.post_rot_quaternion.x, bone.post_rot_quaternion.y, bone.post_rot_quaternion.z, bone.post_rot_quaternion.a };
		FbxQuaternion bind_rot_quat{ bone.bind_pose_rotation.x, bone.bind_pose_rotation.y, bone.bind_pose_rotation.z, bone.bind_pose_rotation.a };

		auto full_rot = post_rot_quat * bind_rot_quat * pre_rot_quat;

		node_ptr->SetPreRotation(FbxNode::eSourcePivot, pre_rot_quat.DecomposeSphericalXYZ());

		node_ptr->SetPostTargetRotation(post_rot_quat.DecomposeSphericalXYZ());

		FbxMatrix  lTransformMatrix;
		node_ptr->LclRotation.Set(full_rot.DecomposeSphericalXYZ());
		node_ptr->LclTranslation.Set(FbxDouble3{ bone.bind_pose_transform.x, bone.bind_pose_transform.y, bone.bind_pose_transform.z });
		FbxVector4 lT, lR, lS;
		lT = FbxVector4(node_ptr->LclTranslation.Get());
		lR = FbxVector4(node_ptr->LclRotation.Get());
		lS = FbxVector4(node_ptr->LclScaling.Get());

		lTransformMatrix.SetTRS(lT, lR, lS);

		boneListing.push_back(bone);
		nodes[boneCounter] = node_ptr;
	}

	// build hierarchy
	for (uint32_t bone_num = 0; bone_num < boneCount; ++bone_num)
	{
		Skeleton::Bone& bone = getBone(bone_num, 0);
		auto idx_parent = bone.parent_idx;
		if (idx_parent == -1)
			parent_ptr->AddChild(nodes[bone_num]);
		else
		{
			auto& parent = nodes[idx_parent];
			parent->AddChild(nodes[bone_num]);
		}
	}

	// build bind pose
	auto mesh_attr = reinterpret_cast<FbxGeometry*>(parent_ptr->GetNodeAttribute());
	FbxSkin *skin = FbxSkin::Create(scene_ptr, parent_ptr->GetName());
	skin->SetSkinningType(FbxSkin::EType::eLinear);

	auto xmatr = parent_ptr->EvaluateGlobalTransform();
	FbxAMatrix link_transform;

	// create clusters
	// create vertex index arrays for clusters
	std::map<std::string, std::vector<std::pair<uint32_t, float>>> cluster_vertices;
	uint32_t counter = 0;
	
	for (int cc = 0; cc < p_CompleteModels.at(0).size(); cc++)
	{
		auto& modelIterator = p_CompleteModels.at(0).at(cc);

		const auto& vertices = modelIterator.get_vertices();
		const auto& mesh_joint_names = modelIterator.get_joint_names();
		uint32_t sizeValue = vertices.size();

		for (uint32_t vertex_num = 0; vertex_num < vertices.size(); ++vertex_num)
		{
			uint32_t finalVertexNum = vertex_num + counter;
			const auto& vertex = vertices[vertex_num];
			for (const auto& weight : vertex.get_weights())
			{
				auto joint_name = mesh_joint_names[weight.first];
				boost::to_lower(joint_name);
				cluster_vertices[joint_name].emplace_back(finalVertexNum, weight.second);
			}
		}

		counter += modelIterator.get_vertices().size();
	}
	
	

	for (uint32_t bone_num = 0; bone_num < boneCount; ++bone_num)
	{
		Skeleton::Bone& bone = getBone(bone_num, 0);
		auto cluster = FbxCluster::Create(scene_ptr, bone.name.c_str());
		cluster->SetLink(nodes[bone_num]);
		cluster->SetLinkMode(FbxCluster::eNormalize);

		auto bone_name = bone.name;
		boost::to_lower(bone_name);

		if (cluster_vertices.find(bone_name) != cluster_vertices.end())
		{
			auto& cluster_vertex_array = cluster_vertices[bone_name];
			for (const auto& vertex_weight : cluster_vertex_array)
				cluster->AddControlPointIndex(vertex_weight.first, vertex_weight.second);
		}

		cluster->SetTransformMatrix(xmatr);
		link_transform = nodes[bone_num]->EvaluateGlobalTransform();
		cluster->SetTransformLinkMatrix(link_transform);

		clusters[bone_num] = cluster;
		skin->AddCluster(cluster);
	}

	mesh_attr->AddDeformer(skin);

	auto pose_ptr = FbxPose::Create(scene_ptr, parent_ptr->GetName());
	pose_ptr->SetIsBindPose(true);

	FbxAMatrix matrix;
	for (uint32_t bone_num = 0; bone_num < boneCount; ++bone_num)
	{
		matrix = nodes[bone_num]->EvaluateGlobalTransform();
		FbxVector4 TranVec = matrix.GetT();
		FbxVector4 RotVec = matrix.GetR();

		pose_ptr->Add(nodes[bone_num], matrix);
	}
	scene_ptr->AddPose(pose_ptr);
	return boneListing;
}
