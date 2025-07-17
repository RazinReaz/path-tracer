# CXX = g++
# CXX_FLAGS = -std=c++17 -Wall -Wextra -I./src -Idependencies/include -Iinclude
CXX = cl
CXX_FLAGS = /std:c++17 /EHsc /W3 /I./src /Idependencies/include /Iinclude /MD
GL_LIBS_LINUX = -lglfw -lGL -ldl
GL_LIBS_WINDOWS = -Ldependencies/lib -lglfw3 -lopengl32 -lgdi32 -luser32 -lkernel32 -lshell32 -lcomdlg32 -ladvapi32 -lwinmm -lws2_32

GL_LIBS = $(GL_LIBS_WINDOWS)

CUDA = nvcc
CUDA_FLAGS = -arch=sm_61 -I./src -Iinclude -Idependencies/include --expt-relaxed-constexpr -Xcompiler="/W3 /EHsc /MD"
CUDA_LIBS = -lcuda -lcudart

# Build output directory for object files
OBJDIR = obj
CUDA_OBJDIR = $(OBJDIR)/cuda
BINDIR = bin

# sources
CPP_SRC = src/opengl/shader.cpp \
		src/opengl/VAO.cpp \
		src/opengl/VBO.cpp \
		src/opengl/EBO.cpp \

# CUDA_SRC = src/ray-tracer/ray.cu\
# 		src/ray-tracer/triangle.cu\

GLAD_SRC = src/glad.c

# tests
TEST_GLFW_SRC = src/test/test_glfw.cpp
TEST_SRC = src/test/test.cpp
LOADER_SRC = src/test/obj_loader.cpp
EBO_SRC = src/test/ebo.cpp
TEXTURE_SRC = src/test/texture.cpp
LIGHT_SRC = src/test/light.cpp

TARGETS = obj_loader test_glfw ebo test texture light

CPP_OBJ = $(CPP_SRC:src/%.cpp=$(OBJDIR)/%.obj)
CUDA_OBJ = $(CUDA_SRC:src/%.cu=$(CUDA_OBJDIR)/%.obj)

GLAD_OBJ = $(GLAD_SRC:src/%.c=$(OBJDIR)/%.obj)

TEST_GLFW_OBJ = $(TEST_GLFW_SRC:src/%.cpp=$(OBJDIR)/%.obj)
TEST_OBJ = $(TEST_SRC:src/%.cpp=$(OBJDIR)/%.obj)
LOADER_OBJ = $(LOADER_SRC:src/%.cpp=$(OBJDIR)/%.obj)
EBO_OBJ = $(EBO_SRC:src/%.cpp=$(OBJDIR)/%.obj)
TEXTURE_OBJ = $(TEXTURE_SRC:src/%.cpp=$(OBJDIR)/%.obj)
LIGHT_OBJ = $(LIGHT_SRC:src/%.cpp=$(OBJDIR)/%.obj)


# Ensure the bin directory exists
$(BINDIR):
	@mkdir -p $(BINDIR)

all: $(BINDIR) $(TARGETS)

# to build the target, we need all object files, but we dont have them yet!
# compile the object files using the %.c:%.cpp rule
# now that they are built, use them (%^) to create the final target ($@)


$(BINDIR)/obj_loader: $(LOADER_OBJ)
	$(CXX) $(CXX_FLAGS) -o $@ $^

$(BINDIR)/test_glfw: $(TEST_GLFW_OBJ) $(GLAD_OBJ)
	$(CXX) $(CXX_FLAGS) -o $@ $^ $(GL_LIBS)

$(BINDIR)/ebo: $(EBO_OBJ) $(GLAD_OBJ) $(CPP_OBJ)
	$(CXX) $(CXX_FLAGS) -o $@ $^ $(GL_LIBS)

$(BINDIR)/test: $(TEST_OBJ) $(GLAD_OBJ)
	$(CXX) $(CXX_FLAGS) -o $@ $^ $(GL_LIBS)

$(BINDIR)/texture: $(TEXTURE_OBJ) $(GLAD_OBJ) $(CPP_OBJ)
	$(CXX) $(CXX_FLAGS) -o $@ $^ $(GL_LIBS)

$(BINDIR)/light: $(LIGHT_OBJ) $(GLAD_OBJ) $(CPP_OBJ) 
	$(CXX) $(CXX_FLAGS) -o $@ $^ $(GL_LIBS)


# @mkdir -p $(dir $@)
$(OBJDIR)/%.obj : src/%.cpp
	@if not exist "$(dir $@)" mkdir "$(dir $@)"
	$(CXX) $(CXX_FLAGS) /c $< /Fo$@

# Rule for CUDA files
$(CUDA_OBJDIR)/%.obj : src/%.cu
	@if not exist "$(dir $@)" mkdir "$(dir $@)"
	$(CUDA) $(CUDA_FLAGS) -c $< -o $@

# just for glad.c since it's a c file
# @mkdir -p $(dir $@)
$(OBJDIR)/%.obj : %.c
	@if not exist "$(dir $@)" mkdir "$(dir $@)"
	$(CXX) $(CXX_FLAGS) /c $< /Fo$@


$(BINDIR)/0.hello.run: src/test-cuda/0.hello.cu
	$(CUDA) -o $@ $< $(GLAD_SRC) $(CUDA_FLAGS) $(CUDA_LIBS) $(GL_LIBS)
$(BINDIR)/1.cuda_opengl_interop.run: src/test-cuda/1.cuda_opengl_interop.cu $(CPP_OBJ)
	$(CUDA) -o $@ $^ $(GLAD_SRC) $(CUDA_FLAGS) $(CUDA_LIBS) $(GL_LIBS)
$(BINDIR)/2.movement.run: src/test-cuda/2.movement.cu $(CPP_OBJ)
	$(CUDA) -o $@ $^ $(GLAD_SRC) $(CUDA_FLAGS) $(CUDA_LIBS) $(GL_LIBS)


run-%: $(BINDIR)/%
	@echo "Running $<..."
	@./$<

run-cuda-%: $(BINDIR)/%.run
	@echo "Running CUDA $<..."
	@./$<

# rm -f $(BINDIR)/* $(OBJDIR)/*.o
clean:
	@if exist "$(BINDIR)" del /Q "$(BINDIR)\*"
	@for /R "$(OBJDIR)" %%f in (*.obj) do del "%%f"