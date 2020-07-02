bl_info = {
    "name": "Neverwinter Nights 2 MDB/GR2 formats (NWN2MDK)",
    "author": "FreshLook",
    "version": (0, 9, 0),
    "blender": (2, 79, 0),
    "location": "File > Import-Export",
    "description": "Neverwinter Nights 2 MDB/GR2 Import/Export",
    "wiki_url": "https://github.com/Arbos/nwn2mdk/wiki",
    "category": "Import-Export",
}


import bpy
from bpy.props import (
        StringProperty,
        BoolProperty,
        FloatProperty,
        CollectionProperty,
        )
from bpy_extras.io_utils import (
        ImportHelper,
        ExportHelper,
        )


class Import(bpy.types.Operator, ImportHelper):
    """Import MDB/GR2"""               # Tooltip for menu items and buttons.
    bl_idname = "import_scene.nwn2mdk" # Unique identifier for buttons and menu items to reference.
    bl_label = "Import MDB/GR2"        # Display name in the interface.
    bl_options = {'UNDO', 'PRESET'}    # Enable undo for the operator.

    filter_glob = StringProperty(default="*.mdb;*.gr2", options={'HIDDEN'})

    files = CollectionProperty(
            name="File Path",
            type=bpy.types.OperatorFileListElement,
            )

    automatic_bone_orientation = BoolProperty(
            name="Automatic Bone Orientation",
            description="Try to align the major bone axis with the bone children",
            default=True,
            )

    def draw(self, context):
        pass

    def execute(self, context):
        if not self.filepath:
            raise Exception("filepath not set")

        import os

        nw2fbx_path = os.path.join(os.path.dirname(os.path.realpath(__file__)), "nw2fbx")
        args = [nw2fbx_path]

        working_dir = os.path.dirname(self.filepath)

        for file in self.files:
            path = os.path.join(working_dir, file.name)
            args.append(path)

        with open(os.path.join(working_dir, "log.txt"), "w") as log:
            import subprocess
            proc = subprocess.Popen(args, stdout=log, cwd=working_dir)
            proc.wait()

        fbx_path = os.path.splitext(args[1])[0]
        fbx_path +=  ".fbx"

        bpy.ops.import_scene.fbx(filepath=fbx_path,
                                 use_image_search=False,
                                 automatic_bone_orientation=self.automatic_bone_orientation)

        return {'FINISHED'}


class Export(bpy.types.Operator, ExportHelper):
    """Export MDB/GR2"""               # Tooltip for menu items and buttons.
    bl_idname = "export_scene.nwn2mdk" # Unique identifier for buttons and menu items to reference.
    bl_label = "Export MDB/GR2"        # Display name in the interface.
    bl_options = {'UNDO', 'PRESET'}    # Enable undo for the operator.

    filename_ext = ".fbx"
    filter_glob = StringProperty(default="*.fbx", options={'HIDDEN'})

    bake_anim = BoolProperty(
            name="Baked Animation",
            description="Export baked keyframe animation",
            default=False,
            )

    bake_anim_simplify_factor = FloatProperty(
            name="Simplify",
            description="How much to simplify baked values (0.0 to disable, the higher the more simplified)",
            min=0.0, max=100.0,  # No simplification to up to 10% of current magnitude tolerance.
            soft_min=0.0, soft_max=10.0,
            default=1.0,  # default: min slope: 0.005, max frame step: 10.
            )

    def draw(self, context):
        pass

    def execute(self, context):
        if not self.filepath:
            raise Exception("filepath not set")

        bpy.ops.export_scene.fbx(filepath=self.filepath,
                                 axis_forward='-Z',
                                 axis_up='Y',
                                 use_tspace=True,
                                 use_custom_props=True,
                                 add_leaf_bones=False,
                                 bake_anim=self.bake_anim,
                                 bake_anim_use_all_bones=False,
                                 bake_anim_use_nla_strips=False,
                                 bake_anim_use_all_actions=False,
                                 bake_anim_force_startend_keying=False,
                                 bake_anim_simplify_factor=self.bake_anim_simplify_factor)

        import os

        fbx2nw_path = os.path.join(os.path.dirname(os.path.realpath(__file__)), "fbx2nw")
        args = [fbx2nw_path, self.filepath]
        working_dir = os.path.dirname(self.filepath)

        with open(os.path.join(working_dir, "log.txt"), "w") as log:
            import subprocess   
            proc = subprocess.Popen(args, stdout=log, cwd=working_dir)
            proc.wait()

        return {'FINISHED'}


class FBX_PT_import_armature(bpy.types.Panel):
    bl_space_type = 'FILE_BROWSER'
    bl_region_type = 'TOOL_PROPS'
    bl_label = "Armature"
    bl_parent_id = "FILE_PT_operator"

    @classmethod
    def poll(cls, context):
        sfile = context.space_data
        operator = sfile.active_operator

        return operator.bl_idname == "IMPORT_SCENE_OT_nwn2mdk"

    def draw(self, context):
        layout = self.layout

        sfile = context.space_data
        operator = sfile.active_operator

        layout.prop(operator, "automatic_bone_orientation"),


class FBX_PT_export_bake_animation(bpy.types.Panel):
    bl_space_type = 'FILE_BROWSER'
    bl_region_type = 'TOOL_PROPS'
    bl_label = "Bake Animation"
    bl_parent_id = "FILE_PT_operator"

    @classmethod
    def poll(cls, context):
        sfile = context.space_data
        operator = sfile.active_operator

        return operator.bl_idname == "EXPORT_SCENE_OT_nwn2mdk"

    def draw_header(self, context):
        sfile = context.space_data
        operator = sfile.active_operator

        self.layout.prop(operator, "bake_anim", text="")

    def draw(self, context):
        layout = self.layout

        sfile = context.space_data
        operator = sfile.active_operator

        layout.enabled = operator.bake_anim
        layout.prop(operator, "bake_anim_simplify_factor")


def menu_func_import(self, context):
    self.layout.operator(Import.bl_idname, text="Neverwinter Nights 2 (MDK) (.mdb/.gr2)")


def menu_func_export(self, context):
    self.layout.operator(Export.bl_idname, text="Neverwinter Nights 2 (MDK) (.mdb/.gr2)")


classes = (
    Import,
    FBX_PT_import_armature,
    Export,
    FBX_PT_export_bake_animation,
)


def register():
    for cls in classes:
        bpy.utils.register_class(cls)

    bpy.types.INFO_MT_file_import.append(menu_func_import)
    bpy.types.INFO_MT_file_export.append(menu_func_export)


def unregister():
    bpy.types.INFO_MT_file_import.remove(menu_func_import)
    bpy.types.INFO_MT_file_export.remove(menu_func_export)

    for cls in classes:
        bpy.utils.unregister_class(cls)


# This allows you to run the script directly from Blender's Text editor
# to test the add-on without having to install it.
if __name__ == "__main__":
    register()