src_dir = "."
obj_dir = "obj"

AddTool(function(s)
	s.cc.flags:Add("-Wall")
	s.cc.flags:Add("-Wextra")
	s.cc.flags_cxx:Add("--std=c++17")
	s.link.libs:Add("GL")
	s.link.libs:Add("GLU")
	s.link.libs:Add("GLEW")
	s.link.libs:Add("glfw")
	s.cc.includes:Add(src_dir)

	s.cc.Output = function(s, input)
		input = input:gsub("^"..src_dir.."/", "")
		return PathJoin(obj_dir, PathBase(input))
	end
end)

s = NewSettings()

src = CollectRecursive(PathJoin(src_dir, "*.cpp"))
obj = Compile(s, src)
bin = Link(s, "voxel_raytracer", obj)
PseudoTarget("compile", bin)
PseudoTarget("c", bin)
DefaultTarget(bin)

AddJob("run", "running '"..bin.."'...", "./"..bin)
AddDependency("run", bin)
