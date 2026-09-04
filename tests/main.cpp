// The entry point of the suite: the startup mode decider, the declarations of
// every test and TEST_LIST. The tests themselves are in test_*.cpp, one file
// per subsystem; see test_helpers.h for how the files fit together.
#include "test_helpers.h"

// The suite has nothing to show. A hidden window keeps the GL context the
// hardware tests need and keeps the screen quiet while they run, whether the
// binary is started by ctest, by hand or by a test name. The environment still
// wins over this, so ARCTIC_HEADLESS=1 ARCTIC_DISABLE_HW=1 runs the suite with
// no window and no GL at all.
StartupMode TestsStartupMode() {
  return StartupMode::kHiddenWindow;
}
ARCTIC_STARTUP_MODE_DECIDER(TestsStartupMode)

// The tests, by file. A test is a non-static void function; a name listed here
// and missing from its file is a link error, which is the check that the two
// lists agree.

// test_unicode.cpp
void test_utf16_to_utf8_spurious_null();
void test_utf32_to_utf8_ascii();
void test_utf32_to_utf8_multibyte();
void test_utf16_to_utf8_bmp();
void test_utf16_to_utf8_surrogate();
void test_utf32_reader_roundtrip();
void test_utf8_codepoint_sizes();
void test_is_utf8_continuation();
void test_utf8_next_char_pos();
void test_utf8_prev_char_pos();

// test_random.cpp
void test_random();
void test_random_seed_determinism();
void test_random_state_continues_the_sequence();
void test_random_state_text_round_trip();

// test_sound.cpp
void test_sound_resample_returns_nullptr();
void test_sound_8bit_stereo_wrong_offset();
void test_sound_8bit_signed_vs_unsigned();

// test_formats.cpp
void test_localization_basic_load();
void test_localization_simple_substitution();
void test_localization_plural_english();
void test_localization_plural_russian();
void test_localization_select();
void test_localization_complex_pattern();
void test_localization_nested_plural_select();
void test_localization_multi_locale_csv();
void test_localization_fallback();
void test_localization_merge_files();
void test_localization_loc_function();
void test_localization_format_pattern_direct();
void test_localization_ordinal_english();
void test_json_parse_string();
void test_json_parse_file();
void test_json_build_and_serialize();
void test_json_type_conversions();
void test_json_iteration();
void test_json_error_handling();
void test_json_modification();
void test_json_comparison();
void test_data_writer_empty_initial_write();
void test_data_writer_multiple_writes_no_overlap();
void test_data_writer_uint16();
void test_data_writer_uint32();
void test_data_writer_uint64();
void test_data_writer_float();
void test_data_writer_mixed_sequence();
void test_data_writer_uint16array();
void test_data_reader_advances_pointer();
void test_data_roundtrip_all_types();
void test_data_roundtrip_arrays();
void test_data_reader_past_end();
void test_data_writer_large_sequence();
void test_csv_roundtrip_separator_in_field();
void test_csv_roundtrip_quotes_in_field();

// test_gl.cpp
void test_gl_program_detaches_shaders_after_link();
void test_gl_uniforms_table_overwrites_a_value();
void test_gl_texture2d_bind_hit_still_activates_the_slot();
void test_gl_texture2d_forgets_a_deleted_name();
void test_gl_framebuffer_forgets_a_deleted_name();
void test_gl_buffer_forgets_a_deleted_name();
void test_gl_texture_cache_identity_and_white();

// test_font.cpp
void test_font_border_survives_colorize();
void test_font_loads_with_border();
void test_ttf_font_loading();
void test_find_system_font();
void test_load_system_font();
void test_font_draw_with_empty_palette();

// test_mesh.cpp
void test_mesh_ply_readline_crlf();
void test_mesh_vertex_attrib_write_overflow();
void test_mesh_extrude_face_covers_all_edges();
void test_mesh_named_elements();
void test_mesh_capacity_is_final();
void test_mesh_expand_every_stream();
void test_mesh_clone_keeps_counts_and_data();
void test_fbx_rejects_unsupported_version();
void test_fbx_rejects_a_buffer_too_short_for_a_header();
void test_fbx_rejects_record_past_end_of_file();
void test_fbx_empty_scene_loads();
void test_fbx_mesh_geometry_is_triangulated();
void test_fbx_material_diffuse_color();
void test_fbx_texture_file_names();
void test_fbx_material_texture_slots();
void test_fbx_material_texture_qualified_property();
void test_fbx_material_ignores_unknown_property();
void test_fbx_video_supplies_missing_texture_file_name();
void test_fbx_video_lowercase_file_name_element();
void test_fbx_video_does_not_override_texture_file_name();
void test_fbx_layered_texture_reaches_material();
void test_fbx_layered_texture_keeps_the_bottom_layer();
void test_fbx_video_behind_layered_texture();
void test_fbx_tolerates_unexpected_connection();
void test_fbx_object_types_and_count();
void test_fbx_global_settings_up_axis();
void test_fbx_global_settings_pass_other_fields_through();
void test_fbx_missing_global_settings_is_not_an_error();
void test_loadfbx_empty_buffer_fails();
void test_loadfbx_scene_without_meshes_fails();
void test_loadfbx_quad_is_two_triangles();
void test_loadfbx_z_up_swaps_y_and_z();
void test_loadfbx_two_meshes_are_two_parts();
void test_resolve_asset_path_finds_basename_and_stem();
void test_loadfbx_skin_and_cluster();
void test_loadfbx_animation_curve_keys();
void test_loadfbx_texture_path_unresolved();

// test_physics.cpp
void test_sphere_vs_triangle_degenerate_and_inside();
void test_sphere_vs_triangle_static_overlap();
void test_sphere_vs_triangle_swept_face();
void test_sphere_vs_triangle_swept_edge_vertex_and_overlap();
void test_sphere_vs_triangles_earliest_and_empty();
void test_line_segment_pierces_triangle();
void test_swept_sphere_resting_face_does_not_fall_through();
void test_slope_into_plane_slides_along_tangent();
void test_hover_racer_log_drop_through_rock_slope();
void test_hover_racer_log_zero_vel_drop_through_slope();
void test_hover_racer_log_stall_on_shallow_slope();
void test_hover_racer_log_stall_on_slope_crease();
void test_hover_racer_log_stall_on_shared_edge();
void test_hover_racer_log_slope_hover_spring_fights_sit();
void test_hover_racer_log_still_y_jitter_on_slope();
void test_hover_racer_log_tiny_reverse_y_drop();
void test_hover_racer_log_tiny_forward_hover_dip();
void test_hover_racer_log_downhill_stair_shake();
void test_hover_racer_log_crest_fall_instead_of_level();
void test_hover_racer_log_wall_corner_dead_stop();
void test_hover_racer_log_wall_ledge_shake();
void test_hover_racer_log_wall_edge_side_snag();
void test_swept_sphere_hits_ceiling_from_below();
void test_classify_sphere_pass_through();
void test_physics_collide_soup_broadphase_completeness();
void test_physics_collide_soup_wide_aabb_not_dropped();
void test_physics_manifold_feature_stability();
void test_physics_solver_corner_order_invariance_and_pgs();
void test_physics_solver_coplanar_wedge_stops_cleanly();
void test_physics_solver_duplicate_constraint();
void test_physics_world_fixture_corner_dead_stop();
void test_physics_world_fixture_edge_snag();
void test_physics_world_fixture_ledge_shake();
void test_physics_sphere_body_reacquires_steep_floor_gap();
void test_physics_sphere_body_hovering_over_floor_is_not_sitting();
void test_physics_world_fixture_floor_climb_collapse();
void test_physics_sphere_body_rolls_over_road_seam_skirt();
void test_physics_sphere_body_step_above_center_still_blocks();
void test_physics_drop_probe_conventions_at_a_seam();
void test_physics_sweep_backoff_is_a_distance_not_a_fraction();
void test_physics_step_report_merges_substeps();
void test_physics_body_separation_stays_out_of_the_mesh();
void test_physics_world_query_conventions();
void test_physics_broad_phase_offers_every_overlapping_triangle();
void test_physics_fixture_builder_catches_bad_geometry();
void test_physics_support_measures_height_above_floor();

// test_math.cpp
void test_quat_to_mat33f_sign();
void test_quat_to_partial_mat33f_sign();
void test_quat_slerp_unnormalized();
void test_quat_slerp_negative_dot();
void test_transform3f_inverse();
void test_transform3f_inverse_composition();
void test_skeleton_bone0_cannot_be_parent();
void test_skeleton_root_not_orphaned();
void test_rotation_x_vs_axis_angle();
void test_rotation_y_vs_axis_angle();
void test_rotation_z_vs_axis_angle();
void test_rotation_xy_composition_vs_axis_angle();
void test_quat_to_axis_angle_acos_out_of_range();
void test_quat_to_axis_angle_normalized_roundtrip();
void test_transform3f_scale_affects_point();
void test_transform3f_scale_affects_transform_composition();
void test_transform3f_inverse_respects_scale();
void test_quat_matrix_vs_axis_angle();
void test_rotation_xyz_vs_axis_angle_consistency();
void test_euler4_equals_composition();
void test_quat_tomat33_vs_setrotationquat();
void test_extract_euler_roundtrip_euler4();
void test_extract_euler_pure_x();
void test_extract_euler_pure_y();
void test_extract_euler_pure_z();
void test_extract_euler_gimbal_lock();
void test_extract_euler_ignores_translation();
void test_extract_euler_gimbal_branch_reads_m11();
void test_rotation_matrices_are_orthogonal();
void test_translation_transforms_point();
void test_lookat_orthonormal();
void test_lookat_degenerate_returns_identity();
void test_perspective_y_equals_cot_half_fovy();
void test_perspective_matches_frustum_perspective();
void test_perspective_tiled_matches_perspective();
void test_ortho_symmetric_corners();
void test_ortho_asymmetric_corners();
void test_ortho_center_maps_to_origin();
void test_ortho_consistent_with_perspective();

// test_platform.cpp
void test_file_operations();
void test_canonicalize_argv_path();
void test_describe_file_path();
void test_file_existence_and_current_directory();
void test_window_title_names_the_program();
void test_loopback_sockets_connect_talk_and_close();
void test_startup_mode_decider();
void test_log_file_is_findable_clearable_and_rotated();
void test_main_window_close_handler();
void test_canonicalize_nonexistent_path();
void test_canonicalize_before_and_after_create();
void test_relative_path_from_to();

// test_gui.cpp
void test_panel_anchor_no_negative_size();
void test_editbox_reports_text_change_and_edit_done();
void test_editbox_accepts_any_layout();
void test_panel_is_inside_answers_for_the_interface();
void test_panel_set_clickable_makes_a_window();
void test_hidden_editbox_and_progressbar_are_not_there();
void test_is_inside_counts_editbox_and_scrollbar();
void test_panel_anchor_and_dock_follow_the_calls();
void test_tab_skips_hidden_panels();
void test_hidden_and_disabled_panels_never_handle_input();
void test_panel_clips_children();
void test_text_word_wrap();
void test_editbox_read_only_password_placeholder();
void test_image_scale_modes();
void test_slider();
void test_radio_buttons();
void test_listbox();
void test_dropdown();
void test_tab_control();
void test_tooltip();
void test_tooltip_stays_on_screen();
void test_focused_editbox_keeps_button_hotkeys_quiet();
void test_button_and_checkbox_need_a_press_on_themselves_to_click();

// test_input.cpp
void test_typed_characters_of_a_message();
void test_typed_text_and_generic_modifiers();
void test_mouse_arrives_in_backbuffer_pixels();
void test_key_down_seconds_measures_the_hold();

// test_sprite.cpp
void test_radix_sort();
void test_radix_sort_correctness();
void test_rgb();
void test_tga_oom();
void test_rgba();
void test_sprite_save_png();
void test_sprite_load_png_round_trip();
void test_sprite_load_png_palette();
void test_sprite_load_png_refuses_garbage();
void test_colorize_blend_rb_vs_g_inconsistency();
void test_colorize_blend_full_vs_partial_alpha_discontinuity();
void test_top_left_coordinate_helpers();
void test_is_point_in_sprite_follows_the_drawing();
void test_hw_rectangle_fill_and_composition_order();
void test_sprite_reference_zero_size();
void test_hw_sprite_subregion_draws_correctly();

TEST_LIST = {
//  {"Tga oom", test_tga_oom},
  {"Rgba", test_rgba},
  {"Radix sort", test_radix_sort},
  {"Radix sort correctness", test_radix_sort_correctness},
  {"Rgb", test_rgb},
  {"File operations", test_file_operations},
  {"Random generation", test_random},
  {"Localization basic load", test_localization_basic_load},
  {"Localization simple substitution", test_localization_simple_substitution},
  {"Localization plural English", test_localization_plural_english},
  {"Localization plural Russian", test_localization_plural_russian},
  {"Localization select", test_localization_select},
  {"Localization complex pattern", test_localization_complex_pattern},
  {"Localization nested plural/select", test_localization_nested_plural_select},
  {"Localization multi-locale CSV", test_localization_multi_locale_csv},
  {"Localization fallback", test_localization_fallback},
  {"Localization merge files", test_localization_merge_files},
  {"Localization Loc() function", test_localization_loc_function},
  {"Localization FormatPattern direct", test_localization_format_pattern_direct},
  {"Localization ordinal English", test_localization_ordinal_english},
  {"TTF font loading", test_ttf_font_loading},
  {"Find system font", test_find_system_font},
  {"Load system font", test_load_system_font},
  {"JSON parse string", test_json_parse_string},
  {"JSON parse file", test_json_parse_file},
  {"JSON build and serialize", test_json_build_and_serialize},
  {"JSON type conversions", test_json_type_conversions},
  {"JSON iteration", test_json_iteration},
  {"JSON error handling", test_json_error_handling},
  {"JSON modification", test_json_modification},
  {"JSON comparison", test_json_comparison},
  {"DataWriter empty initial write", test_data_writer_empty_initial_write},
  {"DataWriter multiple writes no overlap", test_data_writer_multiple_writes_no_overlap},
  {"DataWriter Ui16", test_data_writer_uint16},
  {"DataWriter Ui32", test_data_writer_uint32},
  {"DataWriter Ui64", test_data_writer_uint64},
  {"DataWriter float", test_data_writer_float},
  {"DataWriter mixed sequence", test_data_writer_mixed_sequence},
  {"DataWriter Ui16 array", test_data_writer_uint16array},
  {"DataReader advances pointer", test_data_reader_advances_pointer},
  {"Data roundtrip all types", test_data_roundtrip_all_types},
  {"Data roundtrip arrays", test_data_roundtrip_arrays},
  {"DataReader past end", test_data_reader_past_end},
  {"DataWriter large sequence", test_data_writer_large_sequence},
  {"Sound resample returns nullptr", test_sound_resample_returns_nullptr},
  {"Sound 8-bit stereo wrong offset", test_sound_8bit_stereo_wrong_offset},
  {"Sound 8-bit signed vs unsigned", test_sound_8bit_signed_vs_unsigned},
  {"Quaternion ToMat33F sign error", test_quat_to_mat33f_sign},
  {"Quaternion ToPartialMatrix33F sign error", test_quat_to_partial_mat33f_sign},
  {"Quaternion slerp uses unnormalized inputs", test_quat_slerp_unnormalized},
  {"Quaternion slerp negative dot product", test_quat_slerp_negative_dot},
  {"Transform3F Inverse round-trip", test_transform3f_inverse},
  {"Transform3F Inverse composition is identity", test_transform3f_inverse_composition},
  {"Skeleton bone 0 cannot be parent", test_skeleton_bone0_cannot_be_parent},
  {"Skeleton root not orphaned after AddBone(0)", test_skeleton_root_not_orphaned},
  {"Utf16ToUtf8 spurious null byte", test_utf16_to_utf8_spurious_null},
  {"Utf32ToUtf8 ASCII", test_utf32_to_utf8_ascii},
  {"Utf32ToUtf8 multibyte", test_utf32_to_utf8_multibyte},
  {"Utf16ToUtf8 BMP codepoint", test_utf16_to_utf8_bmp},
  {"Utf16ToUtf8 surrogate pair", test_utf16_to_utf8_surrogate},
  {"Utf32Reader round-trip", test_utf32_reader_roundtrip},
  {"Utf8Codepoint sizes", test_utf8_codepoint_sizes},
  {"IsUtf8Continuation", test_is_utf8_continuation},
  {"Utf8NextCharPos", test_utf8_next_char_pos},
  {"Utf8PrevCharPos", test_utf8_prev_char_pos},
  {"Mesh PLY readline does not strip CRLF", test_mesh_ply_readline_crlf},
  {"Mesh vertex attrib write overflow", test_mesh_vertex_attrib_write_overflow},
  {"Mesh extrude face covers all edges", test_mesh_extrude_face_covers_all_edges},
  {"Mesh vertex elements keep attribute names", test_mesh_named_elements},
  {"Mesh capacity from Init is final", test_mesh_capacity_is_final},
  {"Mesh Expand grows every stream", test_mesh_expand_every_stream},
  {"SetRandomSeed makes the sequence repeat", test_random_seed_determinism},
  {"Random state continues the sequence", test_random_state_continues_the_sequence},
  {"Random state survives a trip through text", test_random_state_text_round_trip},
  {"Sprite saves png and tga", test_sprite_save_png},
  {"Sprite reads back a png it saved", test_sprite_load_png_round_trip},
  {"Sprite reads a palette png with transparency", test_sprite_load_png_palette},
  {"Sprite refuses a broken png", test_sprite_load_png_refuses_garbage},
  {"SetRotationX vs SetRotationAxisAngle4", test_rotation_x_vs_axis_angle},
  {"SetRotationY vs SetRotationAxisAngle4", test_rotation_y_vs_axis_angle},
  {"SetRotationZ vs SetRotationAxisAngle4 (control)", test_rotation_z_vs_axis_angle},
  {"Rotation XY composition vs AxisAngle4", test_rotation_xy_composition_vs_axis_angle},
  {"Colorize blend R/B vs G inconsistency", test_colorize_blend_rb_vs_g_inconsistency},
  {"Colorize blend full vs partial alpha discontinuity", test_colorize_blend_full_vs_partial_alpha_discontinuity},
  {"Quaternion ToAxisAngle acos out of range", test_quat_to_axis_angle_acos_out_of_range},
  {"Quaternion ToAxisAngle normalized round-trip", test_quat_to_axis_angle_normalized_roundtrip},
  {"Transform3F scale affects point", test_transform3f_scale_affects_point},
  {"Transform3F scale affects composition", test_transform3f_scale_affects_transform_composition},
  {"Transform3F Inverse respects scale", test_transform3f_inverse_respects_scale},
  {"Sprite Reference zero-size source", test_sprite_reference_zero_size},
  {"HW sprite sub-region draws correctly", test_hw_sprite_subregion_draws_correctly},
  {"GlProgram detaches shaders after link", test_gl_program_detaches_shaders_after_link},
  {"UniformsTable overwrites a value", test_gl_uniforms_table_overwrites_a_value},
  {"GlTexture2D bind hit still activates the slot", test_gl_texture2d_bind_hit_still_activates_the_slot},
  {"GlTexture2D forgets a deleted name", test_gl_texture2d_forgets_a_deleted_name},
  {"GlFramebuffer forgets a deleted name", test_gl_framebuffer_forgets_a_deleted_name},
  {"GlBuffer forgets a deleted name", test_gl_buffer_forgets_a_deleted_name},
  {"Font draws with an empty palette", test_font_draw_with_empty_palette},
  {"Mesh Clone keeps counts and data", test_mesh_clone_keeps_counts_and_data},
  {"Quat matrix vs AxisAngle consistency", test_quat_matrix_vs_axis_angle},
  {"Rotation XYZ vs AxisAngle consistency", test_rotation_xyz_vs_axis_angle_consistency},
  {"Euler4 equals Rz*Ry*Rx", test_euler4_equals_composition},
  {"Quat ToMat33F vs SetRotationQuat 4x4", test_quat_tomat33_vs_setrotationquat},
  {"ExtractEuler roundtrip Euler4", test_extract_euler_roundtrip_euler4},
  {"ExtractEuler pure X rotation", test_extract_euler_pure_x},
  {"ExtractEuler pure Y rotation", test_extract_euler_pure_y},
  {"ExtractEuler pure Z rotation", test_extract_euler_pure_z},
  {"ExtractEuler gimbal lock", test_extract_euler_gimbal_lock},
  {"ExtractEuler ignores translation", test_extract_euler_ignores_translation},
  {"ExtractEuler gimbal branch reads m11", test_extract_euler_gimbal_branch_reads_m11},
  {"All rotation matrices are orthogonal", test_rotation_matrices_are_orthogonal},
  {"Translation transforms point correctly", test_translation_transforms_point},
  {"SetLookat produces orthonormal basis", test_lookat_orthonormal},
  {"SetLookat degenerate returns identity", test_lookat_degenerate_returns_identity},
  {"CSV round-trip: separator in field", test_csv_roundtrip_separator_in_field},
  {"CSV round-trip: quotes in field", test_csv_roundtrip_quotes_in_field},
  {"Panel anchor: no negative size on parent shrink", test_panel_anchor_no_negative_size},
  {"SetPerspective y == cot(fovy/2)", test_perspective_y_equals_cot_half_fovy},
  {"SetPerspective matches SetFrustumPerspective", test_perspective_matches_frustum_perspective},
  {"SetPerspectiveTiled identity matches SetPerspective", test_perspective_tiled_matches_perspective},
  {"SetOrtho symmetric maps corners to NDC", test_ortho_symmetric_corners},
  {"SetOrtho asymmetric maps corners to NDC", test_ortho_asymmetric_corners},
  {"SetOrtho center maps to origin", test_ortho_center_maps_to_origin},
  {"SetOrtho consistent with Perspective at z=near", test_ortho_consistent_with_perspective},
  {"CanonicalizePath non-existent path", test_canonicalize_nonexistent_path},
  {"CanonicalizePath before and after file create", test_canonicalize_before_and_after_create},
  {"RelativePathFromTo ignores a trailing slash", test_relative_path_from_to},
  {"CanonicalizeArgvPath uses the startup directory", test_canonicalize_argv_path},
  {"DescribeFilePath explains a missing file", test_describe_file_path},
  {"DoesFileExist and ChangeCurrentDirectory", test_file_existence_and_current_directory},
  {"Font border survives colorize", test_font_border_survives_colorize},
  {"Font loaders apply the border", test_font_loads_with_border},
  {"Editbox reports text change and edit done", test_editbox_reports_text_change_and_edit_done},
  {"Editbox takes text from any keyboard layout", test_editbox_accepts_any_layout},
  {"Typed characters of a message", test_typed_characters_of_a_message},
  {"Typed text and generic modifiers", test_typed_text_and_generic_modifiers},
  {"Top left coordinate helpers", test_top_left_coordinate_helpers},
  {"IsPointInSprite follows the drawing", test_is_point_in_sprite_follows_the_drawing},
  {"Mouse arrives in backbuffer pixels", test_mouse_arrives_in_backbuffer_pixels},
  {"Hardware rectangle fill and composition order", test_hw_rectangle_fill_and_composition_order},
  {"Window title names the program", test_window_title_names_the_program},
  {"KeyDownSeconds measures the hold", test_key_down_seconds_measures_the_hold},
  {"Loopback sockets connect, talk and close",
    test_loopback_sockets_connect_talk_and_close},
  {"Panel IsInside answers whose click it is",
    test_panel_is_inside_answers_for_the_interface},
  {"Panel SetClickable makes a window", test_panel_set_clickable_makes_a_window},
  {"Hidden editbox and progressbar are not there",
    test_hidden_editbox_and_progressbar_are_not_there},
  {"IsInside counts editbox and scrollbar",
    test_is_inside_counts_editbox_and_scrollbar},
  {"Panel anchor and dock follow the calls",
    test_panel_anchor_and_dock_follow_the_calls},
  {"Tab skips hidden panels", test_tab_skips_hidden_panels},
  {"Panel clips children", test_panel_clips_children},
  {"Text word wrap", test_text_word_wrap},
  {"Editbox read-only, password and placeholder",
    test_editbox_read_only_password_placeholder},
  {"Image scale modes", test_image_scale_modes},
  {"Slider", test_slider},
  {"Radio buttons", test_radio_buttons},
  {"ListBox", test_listbox},
  {"Dropdown", test_dropdown},
  {"TabControl", test_tab_control},
  {"Tooltip", test_tooltip},
  {"Tooltip stays on screen", test_tooltip_stays_on_screen},
  {"Focused editbox keeps button hotkeys quiet",
    test_focused_editbox_keeps_button_hotkeys_quiet},
  {"Button and checkbox need a press on themselves to click",
    test_button_and_checkbox_need_a_press_on_themselves_to_click},
  {"Hidden and disabled panels never handle input",
    test_hidden_and_disabled_panels_never_handle_input},
  {"Startup mode decider is asked at startup", test_startup_mode_decider},
  {"Log file is findable, clearable and rotated",
      test_log_file_is_findable_clearable_and_rotated},
  {"Main window close handler decides the exit", test_main_window_close_handler},
  {"FBX rejects an unsupported version", test_fbx_rejects_unsupported_version},
  {"FBX rejects a buffer too short for a header", test_fbx_rejects_a_buffer_too_short_for_a_header},
  {"FBX rejects a record past the end of file", test_fbx_rejects_record_past_end_of_file},
  {"FBX loads a scene without objects", test_fbx_empty_scene_loads},
  {"FBX triangulates mesh geometry", test_fbx_mesh_geometry_is_triangulated},
  {"FBX reads the material diffuse color", test_fbx_material_diffuse_color},
  {"FBX reads the texture file names", test_fbx_texture_file_names},
  {"FBX fills the material texture slots", test_fbx_material_texture_slots},
  {"FBX accepts a qualified texture property", test_fbx_material_texture_qualified_property},
  {"FBX ignores an unknown texture property", test_fbx_material_ignores_unknown_property},
  {"FBX takes a missing texture path from the video", test_fbx_video_supplies_missing_texture_file_name},
  {"FBX reads a lowercase video Filename", test_fbx_video_lowercase_file_name_element},
  {"FBX keeps the texture path over the video one", test_fbx_video_does_not_override_texture_file_name},
  {"FBX resolves a layered texture for a material", test_fbx_layered_texture_reaches_material},
  {"FBX keeps the bottom layer of a layered texture", test_fbx_layered_texture_keeps_the_bottom_layer},
  {"FBX resolves video through a layered texture", test_fbx_video_behind_layered_texture},
  {"FBX tolerates an unusable connection", test_fbx_tolerates_unexpected_connection},
  {"FBX reports the object types it parsed", test_fbx_object_types_and_count},
  {"FBX maps the file up axis onto the enum", test_fbx_global_settings_up_axis},
  {"FBX passes the other global settings through", test_fbx_global_settings_pass_other_fields_through},
  {"FBX accepts a file without global settings", test_fbx_missing_global_settings_is_not_an_error},
  {"LoadFbx rejects an empty buffer", test_loadfbx_empty_buffer_fails},
  {"LoadFbx rejects a scene without meshes", test_loadfbx_scene_without_meshes_fails},
  {"LoadFbx turns a quad into two triangles", test_loadfbx_quad_is_two_triangles},
  {"LoadFbx maps Z-up onto Y-up", test_loadfbx_z_up_swaps_y_and_z},
  {"LoadFbx keeps two materials as two parts", test_loadfbx_two_meshes_are_two_parts},
  {"ResolveAssetPath finds a file by stem", test_resolve_asset_path_finds_basename_and_stem},
  {"GlTextureCache returns the same pointer", test_gl_texture_cache_identity_and_white},
  {"LoadFbx copies skin weights and inverse bind", test_loadfbx_skin_and_cluster},
  {"LoadFbx copies animation curve keys", test_loadfbx_animation_curve_keys},
  {"LoadFbx keeps an unresolved texture path", test_loadfbx_texture_path_unresolved},
  {"Sphere vs triangle degenerate and inside", test_sphere_vs_triangle_degenerate_and_inside},
  {"Sphere vs triangle static overlap", test_sphere_vs_triangle_static_overlap},
  {"Sphere vs triangle swept face", test_sphere_vs_triangle_swept_face},
  {"Sphere vs triangle swept edge vertex overlap", test_sphere_vs_triangle_swept_edge_vertex_and_overlap},
  {"Sphere vs triangles earliest and camera boom", test_sphere_vs_triangles_earliest_and_empty},
  {"Line segment pierces triangle", test_line_segment_pierces_triangle},
  {"Swept sphere resting face does not fall through", test_swept_sphere_resting_face_does_not_fall_through},
  {"Slope into-plane remainder slides along tangent", test_slope_into_plane_slides_along_tangent},
  {"Hover racer log drop through rock slope", test_hover_racer_log_drop_through_rock_slope},
  {"Hover racer log zero-vel drop through slope", test_hover_racer_log_zero_vel_drop_through_slope},
  {"Hover racer log stall on shallow slope", test_hover_racer_log_stall_on_shallow_slope},
  {"Hover racer log stall on slope crease", test_hover_racer_log_stall_on_slope_crease},
  {"Hover racer log stall on shared edge", test_hover_racer_log_stall_on_shared_edge},
  {"Hover racer log slope hover spring fights sit", test_hover_racer_log_slope_hover_spring_fights_sit},
  {"Hover racer log still Y jitter on slope", test_hover_racer_log_still_y_jitter_on_slope},
  {"Hover racer log tiny reverse Y drop", test_hover_racer_log_tiny_reverse_y_drop},
  {"Hover racer log tiny forward hover dip", test_hover_racer_log_tiny_forward_hover_dip},
  {"Hover racer log downhill stair shake", test_hover_racer_log_downhill_stair_shake},
  {"Hover racer log crest fall instead of level", test_hover_racer_log_crest_fall_instead_of_level},
  {"Hover racer log wall corner dead stop", test_hover_racer_log_wall_corner_dead_stop},
  {"Hover racer log wall ledge shake", test_hover_racer_log_wall_ledge_shake},
  {"Hover racer log wall edge side snag",
      test_hover_racer_log_wall_edge_side_snag},
  {"Swept sphere hits ceiling from below", test_swept_sphere_hits_ceiling_from_below},
  {"Classify sphere pass through", test_classify_sphere_pass_through},
  {"Physics collide soup broadphase completeness",
      test_physics_collide_soup_broadphase_completeness},
      {"Physics collide soup wide AABB not dropped",
          test_physics_collide_soup_wide_aabb_not_dropped},
  {"Physics manifold feature stability",
      test_physics_manifold_feature_stability},
  {"Physics solver corner order invariance and PGS",
      test_physics_solver_corner_order_invariance_and_pgs},
  {"Physics solver coplanar wedge stops cleanly",
      test_physics_solver_coplanar_wedge_stops_cleanly},
  {"Physics solver duplicate constraint",
      test_physics_solver_duplicate_constraint},
  {"Physics world fixture corner dead stop",
      test_physics_world_fixture_corner_dead_stop},
  {"Physics world fixture edge snag",
      test_physics_world_fixture_edge_snag},
  {"Physics world fixture ledge shake",
      test_physics_world_fixture_ledge_shake},
  {"Physics world fixture floor climb collapse",
      test_physics_world_fixture_floor_climb_collapse},
  {"Physics sphere body rolls over road seam skirt",
      test_physics_sphere_body_rolls_over_road_seam_skirt},
  {"Physics sphere body step above center still blocks",
      test_physics_sphere_body_step_above_center_still_blocks},
  {"Physics drop probe conventions at a seam",
      test_physics_drop_probe_conventions_at_a_seam},
  {"Physics sweep backoff is a distance not a fraction",
      test_physics_sweep_backoff_is_a_distance_not_a_fraction},
  {"Physics step report merges substeps",
      test_physics_step_report_merges_substeps},
  {"Physics body separation stays out of the mesh",
      test_physics_body_separation_stays_out_of_the_mesh},
  {"Physics fixture builder catches bad geometry",
      test_physics_fixture_builder_catches_bad_geometry},
  {"Physics world query conventions",
      test_physics_world_query_conventions},
  {"Physics broad phase offers every overlapping triangle",
      test_physics_broad_phase_offers_every_overlapping_triangle},
  {"Physics support measures height above floor",
      test_physics_support_measures_height_above_floor},
  {"Physics sphere body reacquires steep floor gap",
      test_physics_sphere_body_reacquires_steep_floor_gap},
  {"Physics sphere body hovering over floor is not sitting",
      test_physics_sphere_body_hovering_over_floor_is_not_sitting},
  {0}
};
