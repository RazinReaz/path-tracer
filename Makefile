CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -I./src -Idependencies/include -Iinclude
GL_LIBS = -lglfw -lGL -ldl

# Build output directory for object files
OBJDIR = obj
BINDIR = bin

# sources
COMMON_SRC = src/math/vector3f.cpp \
		src/graphics/shader.cpp \
		src/graphics/VAO.cpp \
		src/graphics/VBO.cpp \
		src/graphics/EBO.cpp \
		src/graphics/camera.cpp

GLAD_SRC = src/glad.c
MAIN_SRC = src/main.cpp

TEST_GLFW_SRC = src/test/test_glfw.cpp
TEST_SRC = src/test/test.cpp
LOADER_SRC = src/test/obj_loader.cpp
EBO_SRC = src/test/ebo.cpp
TEXTURE_SRC = src/test/texture.cpp

TARGETS = main obj_loader test_glfw ebo test texture

COMMON_OBJ = $(COMMON_SRC:src/%.cpp=$(OBJDIR)/%.o)
GLAD_OBJ = $(GLAD_SRC:src/%.cpp=$(OBJDIR)/%.o)
MAIN_OBJ = $(MAIN_SRC:src/%.cpp=$(OBJDIR)/%.o)

TEST_GLFW_OBJ = $(TEST_GLFW_SRC:src/%.cpp=$(OBJDIR)/%.o)
TEST_OBJ = $(TEST_SRC:src/%.cpp=$(OBJDIR)/%.o)
LOADER_OBJ = $(LOADER_SRC:src/%.cpp=$(OBJDIR)/%.o)
EBO_OBJ = $(EBO_SRC:src/%.cpp=$(OBJDIR)/%.o)
TEXTURE_OBJ = $(TEXTURE_SRC:src/%.cpp=$(OBJDIR)/%.o)


# Ensure the bin directory exists
$(BINDIR):
	@mkdir -p $(BINDIR)

all: $(BINDIR) $(TARGETS)

# to build the target, we need all object files, but we dont have them yet!
# compile the object files using the %.c:%.cpp rule
# now that they are built, use them (%^) to create the final target ($@)

$(BINDIR)/main: $(MAIN_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BINDIR)/obj_loader: $(LOADER_OBJ) $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BINDIR)/test_glfw: $(TEST_GLFW_OBJ) $(GLAD_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(GL_LIBS)

$(BINDIR)/ebo: $(EBO_OBJ) $(GLAD_OBJ) $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(GL_LIBS)

$(BINDIR)/test: $(TEST_OBJ) $(GLAD_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(GL_LIBS)

$(BINDIR)/texture: $(TEXTURE_OBJ) $(GLAD_OBJ) $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(GL_LIBS)


$(OBJDIR)/%.o : src/%.cpp
	@mkdir -p $(dir $@) 
	$(CXX) $(CXXFLAGS) -c $< -o $@

# just for glad.c since it's a c file
$(OBJDIR)/%.o : %.c
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

run-%: $(BINDIR)/%
	@echo "Running $<..."
	@./$<

clean:
	rm -f $(BINDIR)/* $(OBJDIR)/*.o
