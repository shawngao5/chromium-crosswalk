# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'chromium_code': 1,
  },
  'targets': [
    {
      # GN version: //ui/base
      'target_name': 'ui_base',
      'type': '<(component)',
      'dependencies': [
        '../../base/base.gyp:base',
        '../../base/base.gyp:base_i18n',
        '../../base/base.gyp:base_static',
        '../../base/third_party/dynamic_annotations/dynamic_annotations.gyp:dynamic_annotations',
        '../../net/net.gyp:net',
        '../../skia/skia.gyp:skia',
        '../../third_party/icu/icu.gyp:icui18n',
        '../../third_party/icu/icu.gyp:icuuc',
        '../../url/url.gyp:url_lib',
        '../events/events.gyp:events',
        '../events/events.gyp:events_base',
        '../events/platform/events_platform.gyp:events_platform',
        '../gfx/gfx.gyp:gfx',
        '../gfx/gfx.gyp:gfx_geometry',
        '../resources/ui_resources.gyp:ui_resources',
        '../strings/ui_strings.gyp:ui_strings',
      ],
      'defines': [
        'UI_BASE_IMPLEMENTATION',
      ],
      'export_dependent_settings': [
        '../../net/net.gyp:net',
        '../gfx/gfx.gyp:gfx',
      ],
      'sources' : [
        # Note: file list duplicated in GN build.
        'accelerators/accelerator.cc',
        'accelerators/accelerator.h',
        'accelerators/accelerator_manager.cc',
        'accelerators/accelerator_manager.h',
        'accelerators/menu_label_accelerator_util_linux.cc',
        'accelerators/menu_label_accelerator_util_linux.h',
        'accelerators/platform_accelerator.h',
        'accelerators/platform_accelerator_cocoa.h',
        'accelerators/platform_accelerator_cocoa.mm',
        'android/system_ui_resource_manager.h',
        'android/ui_base_jni_registrar.cc',
        'android/ui_base_jni_registrar.h',
        'android/view_android.cc',
        'android/view_android.h',
        'android/window_android.cc',
        'android/window_android.h',
        'android/window_android_compositor.h',
        'android/window_android_observer.h',
        'base_window.cc',
        'base_window.h',
        'clipboard/clipboard.cc',
        'clipboard/clipboard.h',
        'clipboard/clipboard_android.cc',
        'clipboard/clipboard_android_initialization.h',
        'clipboard/clipboard_aura.cc',
        'clipboard/clipboard_aurax11.cc',
        'clipboard/clipboard_constants.cc',
        'clipboard/clipboard_mac.mm',
        'clipboard/clipboard_types.h',
        'clipboard/clipboard_util_win.cc',
        'clipboard/clipboard_util_win.h',
        'clipboard/clipboard_win.cc',
        'clipboard/custom_data_helper.cc',
        'clipboard/custom_data_helper.h',
        'clipboard/custom_data_helper_linux.cc',
        'clipboard/custom_data_helper_mac.mm',
        'clipboard/scoped_clipboard_writer.cc',
        'clipboard/scoped_clipboard_writer.h',
        'cocoa/animation_utils.h',
        'cocoa/appkit_utils.h',
        'cocoa/appkit_utils.mm',
        'cocoa/base_view.h',
        'cocoa/base_view.mm',
        'cocoa/cocoa_base_utils.h',
        'cocoa/cocoa_base_utils.mm',
        'cocoa/controls/blue_label_button.h',
        'cocoa/controls/blue_label_button.mm',
        'cocoa/controls/hover_image_menu_button.h',
        'cocoa/controls/hover_image_menu_button.mm',
        'cocoa/controls/hover_image_menu_button_cell.h',
        'cocoa/controls/hover_image_menu_button_cell.mm',
        'cocoa/controls/hyperlink_button_cell.h',
        'cocoa/controls/hyperlink_button_cell.mm',
        'cocoa/find_pasteboard.h',
        'cocoa/find_pasteboard.mm',
        'cocoa/flipped_view.h',
        'cocoa/flipped_view.mm',
        'cocoa/focus_tracker.h',
        'cocoa/focus_tracker.mm',
        'cocoa/focus_window_set.h',
        'cocoa/focus_window_set.mm',
        'cocoa/fullscreen_window_manager.h',
        'cocoa/fullscreen_window_manager.mm',
        'cocoa/hover_button.h',
        'cocoa/hover_button.mm',
        'cocoa/hover_image_button.h',
        'cocoa/hover_image_button.mm',
        'cocoa/menu_controller.h',
        'cocoa/menu_controller.mm',
        'cocoa/nib_loading.h',
        'cocoa/nib_loading.mm',
        'cocoa/nsgraphics_context_additions.h',
        'cocoa/nsgraphics_context_additions.mm',
        'cocoa/remote_layer_api.h',
        'cocoa/remote_layer_api.mm',
        'cocoa/tracking_area.h',
        'cocoa/tracking_area.mm',
        'cocoa/underlay_opengl_hosting_window.h',
        'cocoa/underlay_opengl_hosting_window.mm',
        'cocoa/view_description.h',
        'cocoa/view_description.mm',
        'cocoa/window_size_constants.h',
        'cocoa/window_size_constants.mm',
        'cursor/cursor.cc',
        'cursor/cursor.h',
        'cursor/cursor_android.cc',
        'cursor/cursor_loader.h',
        'cursor/cursor_loader_ozone.cc',
        'cursor/cursor_loader_ozone.h',
        'cursor/cursor_loader_win.cc',
        'cursor/cursor_loader_win.h',
        'cursor/cursor_loader_x11.cc',
        'cursor/cursor_loader_x11.h',
        'cursor/cursor_ozone.cc',
        'cursor/cursor_util.cc',
        'cursor/cursor_util.h',
        'cursor/cursor_win.cc',
        'cursor/cursor_x11.cc',
        'cursor/cursors_aura.cc',
        'cursor/cursors_aura.h',
        'cursor/image_cursors.cc',
        'cursor/image_cursors.h',
        'cursor/ozone/bitmap_cursor_factory_ozone.cc',
        'cursor/ozone/bitmap_cursor_factory_ozone.h',
        'default_theme_provider.cc',
        'default_theme_provider.h',
        'default_theme_provider_mac.mm',
        'device_form_factor_android.cc',
        'device_form_factor_android.h',
        'device_form_factor_desktop.cc',
        'device_form_factor_ios.mm',
        'device_form_factor.h',
        'dragdrop/cocoa_dnd_util.h',
        'dragdrop/cocoa_dnd_util.mm',
        'dragdrop/drag_drop_types.h',
        'dragdrop/drag_drop_types_win.cc',
        'dragdrop/drag_source_win.cc',
        'dragdrop/drag_source_win.h',
        'dragdrop/drag_utils.cc',
        'dragdrop/drag_utils.h',
        'dragdrop/drag_utils_aura.cc',
        'dragdrop/drag_utils_mac.mm',
        'dragdrop/drag_utils_win.cc',
        'dragdrop/drop_target_event.cc',
        'dragdrop/drop_target_event.h',
        'dragdrop/drop_target_win.cc',
        'dragdrop/drop_target_win.h',
        'dragdrop/file_info.cc',
        'dragdrop/file_info.h',
        'dragdrop/os_exchange_data.cc',
        'dragdrop/os_exchange_data.h',
        'dragdrop/os_exchange_data_provider_aura.cc',
        'dragdrop/os_exchange_data_provider_aura.h',
        'dragdrop/os_exchange_data_provider_aurax11.cc',
        'dragdrop/os_exchange_data_provider_aurax11.h',
        'dragdrop/os_exchange_data_provider_mac.h',
        'dragdrop/os_exchange_data_provider_mac.mm',
        'dragdrop/os_exchange_data_provider_win.cc',
        'dragdrop/os_exchange_data_provider_win.h',
        'hit_test.h',
        'ime/candidate_window.cc',
        'ime/candidate_window.h',
        'ime/chromeos/character_composer.cc',
        'ime/chromeos/character_composer.h',
        'ime/chromeos/ime_bridge.cc',
        'ime/chromeos/ime_bridge.h',
        'ime/chromeos/ime_keymap.cc',
        'ime/chromeos/ime_keymap.h',
        'ime/chromeos/mock_ime_candidate_window_handler.cc',
        'ime/chromeos/mock_ime_candidate_window_handler.h',
        'ime/chromeos/mock_ime_engine_handler.cc',
        'ime/chromeos/mock_ime_engine_handler.h',
        'ime/chromeos/mock_ime_input_context_handler.cc',
        'ime/chromeos/mock_ime_input_context_handler.h',
        'ime/composition_text.cc',
        'ime/composition_text.h',
        'ime/composition_text_util_pango.cc',
        'ime/composition_text_util_pango.h',
        'ime/composition_underline.h',
        'ime/dummy_input_method_delegate.cc',
        'ime/dummy_input_method_delegate.h',
        'ime/infolist_entry.cc',
        'ime/infolist_entry.h',
        'ime/input_method.h',
        'ime/input_method_auralinux.cc',
        'ime/input_method_auralinux.h',
        'ime/input_method_base.cc',
        'ime/input_method_base.h',
        'ime/input_method_chromeos.cc',
        'ime/input_method_chromeos.h',
        'ime/input_method_delegate.h',
        'ime/input_method_factory.cc',
        'ime/input_method_factory.h',
        'ime/input_method_initializer.cc',
        'ime/input_method_initializer.h',
        'ime/input_method_mac.h',
        'ime/input_method_mac.mm',
        'ime/input_method_minimal.cc',
        'ime/input_method_minimal.h',
        'ime/input_method_observer.h',
        'ime/input_method_win.cc',
        'ime/input_method_win.h',
        'ime/linux/fake_input_method_context.cc',
        'ime/linux/fake_input_method_context.h',
        'ime/linux/fake_input_method_context_factory.cc',
        'ime/linux/fake_input_method_context_factory.h',
        'ime/linux/linux_input_method_context.h',
        'ime/linux/linux_input_method_context_factory.cc',
        'ime/linux/linux_input_method_context_factory.h',
        'ime/mock_input_method.cc',
        'ime/mock_input_method.h',
        'ime/remote_input_method_delegate_win.h',
        'ime/remote_input_method_win.cc',
        'ime/remote_input_method_win.h',
        'ime/text_input_client.cc',
        'ime/text_input_client.h',
        'ime/text_input_focus_manager.cc',
        'ime/text_input_focus_manager.h',
        'ime/text_input_type.h',
        'ime/win/imm32_manager.cc',
        'ime/win/imm32_manager.h',
        'ime/win/tsf_input_scope.cc',
        'ime/win/tsf_input_scope.h',
        'l10n/formatter.cc',
        'l10n/formatter.h',
        'l10n/l10n_font_util.cc',
        'l10n/l10n_font_util.h',
        'l10n/l10n_util.cc',
        'l10n/l10n_util.h',
        'l10n/l10n_util_android.cc',
        'l10n/l10n_util_android.h',
        'l10n/l10n_util_collator.h',
        'l10n/l10n_util_mac.h',
        'l10n/l10n_util_mac.mm',
        'l10n/l10n_util_plurals.cc',
        'l10n/l10n_util_plurals.h',
        'l10n/l10n_util_posix.cc',
        'l10n/l10n_util_win.cc',
        'l10n/l10n_util_win.h',
        'l10n/time_format.cc',
        'l10n/time_format.h',
        'layout.cc',
        'layout.h',
        'layout_mac.mm',
        'models/button_menu_item_model.cc',
        'models/button_menu_item_model.h',
        'models/combobox_model.cc',
        'models/combobox_model.h',
        'models/combobox_model_observer.h',
        'models/dialog_model.cc',
        'models/dialog_model.h',
        'models/list_model.h',
        'models/list_model_observer.h',
        'models/list_selection_model.cc',
        'models/list_selection_model.h',
        'models/menu_model.cc',
        'models/menu_model.h',
        'models/menu_model_delegate.h',
        'models/menu_separator_types.h',
        'models/simple_combobox_model.cc',
        'models/simple_combobox_model.h',
        'models/simple_menu_model.cc',
        'models/simple_menu_model.h',
        'models/table_model.cc',
        'models/table_model.h',
        'models/table_model_observer.h',
        'models/tree_model.cc',
        'models/tree_model.h',
        'models/tree_node_iterator.h',
        'models/tree_node_model.h',
        'nine_image_painter_factory.cc',
        'nine_image_painter_factory.h',
        'resource/data_pack.cc',
        'resource/data_pack.h',
        'resource/resource_bundle.cc',
        'resource/resource_bundle.h',
        'resource/resource_bundle_android.cc',
        'resource/resource_bundle_auralinux.cc',
        'resource/resource_bundle_ios.mm',
        'resource/resource_bundle_mac.mm',
        'resource/resource_bundle_win.cc',
        'resource/resource_bundle_win.h',
        'resource/resource_data_dll_win.cc',
        'resource/resource_data_dll_win.h',
        'resource/resource_handle.h',
        'text/bytes_formatting.cc',
        'text/bytes_formatting.h',
        'theme_provider.cc',
        'theme_provider.h',
        'touch/touch_device.cc',
        'touch/touch_device.h',
        'touch/touch_device_android.cc',
        'touch/touch_device_aurax11.cc',
        'touch/touch_device_ozone.cc',
        'touch/touch_device_win.cc',
        'touch/touch_editing_controller.cc',
        'touch/touch_editing_controller.h',
        'touch/touch_enabled.cc',
        'touch/touch_enabled.h',
        'ui_base_export.h',
        'ui_base_exports.cc',
        'ui_base_paths.cc',
        'ui_base_paths.h',
        'ui_base_switches.cc',
        'ui_base_switches.h',
        'ui_base_switches_util.cc',
        'ui_base_switches_util.h',
        'ui_base_types.cc',
        'ui_base_types.h',
        'view_prop.cc',
        'view_prop.h',
        'webui/jstemplate_builder.cc',
        'webui/jstemplate_builder.h',
        'webui/web_ui_util.cc',
        'webui/web_ui_util.h',
        'win/accessibility_ids_win.h',
        'win/accessibility_misc_utils.cc',
        'win/accessibility_misc_utils.h',
        'win/atl_module.h',
        'win/dpi_setup.cc',
        'win/dpi_setup.h',
        'win/foreground_helper.cc',
        'win/foreground_helper.h',
        'win/hidden_window.cc',
        'win/hidden_window.h',
        'win/hwnd_subclass.cc',
        'win/hwnd_subclass.h',
        'win/internal_constants.cc',
        'win/internal_constants.h',
        'win/lock_state.cc',
        'win/lock_state.h',
        'win/message_box_win.cc',
        'win/message_box_win.h',
        'win/mouse_wheel_util.cc',
        'win/mouse_wheel_util.h',
        'win/open_file_name_win.cc',
        'win/open_file_name_win.h',
        'win/scoped_ole_initializer.cc',
        'win/scoped_ole_initializer.h',
        'win/shell.cc',
        'win/shell.h',
        'win/touch_input.cc',
        'win/touch_input.h',
        'win/window_event_target.cc',
        'win/window_event_target.h',
        'window_open_disposition.cc',
        'window_open_disposition.h',
        'work_area_watcher_observer.h',
        'x/selection_owner.cc',
        'x/selection_owner.h',
        'x/selection_requestor.cc',
        'x/selection_requestor.h',
        'x/selection_utils.cc',
        'x/selection_utils.h',
        'x/x11_foreign_window_manager.cc',
        'x/x11_foreign_window_manager.h',
        'x/x11_menu_list.cc',
        'x/x11_menu_list.h',
        'x/x11_util.cc',
        'x/x11_util.h',
        'x/x11_util_internal.h',
      ],
      'target_conditions': [
        ['OS == "ios"', {
          'sources/': [
            ['include', '^l10n/l10n_util_mac\\.mm$'],
          ],
        }],
      ],
      'conditions': [
        ['OS=="ios"', {
          # iOS only uses a subset of UI.
          'sources/': [
            ['exclude', '\\.(cc|mm)$'],
            ['include', '_ios\\.(cc|mm)$'],
            ['include', '(^|/)ios/'],
            ['include', '^l10n/'],
            ['include', '^layout'],
            ['include', '^resource/'],
            ['include', '^ui_base_'],
          ],
          'link_settings': {
            'libraries': [
              '$(SDKROOT)/System/Library/Frameworks/CoreGraphics.framework',
            ],
          },
        }],
        ['use_aura==1', {
          'sources/': [
            ['exclude', 'clipboard/clipboard_mac.mm'],
            ['exclude', 'layout_mac.mm'],
            ['exclude', 'work_area_watcher_observer.h'],
          ],
        }, {  # use_aura!=1
          'sources!': [
            'cursor/cursor.cc',
            'cursor/cursor.h',
            'cursor/cursor_loader_x11.cc',
            'cursor/cursor_loader_x11.h',
            'cursor/cursor_win.cc',
            'cursor/cursor_x11.cc',
            'x/selection_owner.cc',
            'x/selection_owner.h',
            'x/selection_requestor.cc',
            'x/selection_requestor.h',
            'x/selection_utils.cc',
            'x/selection_utils.h',
          ]
        }],
        ['use_aura==0 or OS!="linux"', {
          'sources!': [
            'resource/resource_bundle_auralinux.cc',
          ],
        }],
        ['use_ozone==1', {
          'dependencies': [
            '../ozone/ozone.gyp:ozone_base',
          ],
        }],
        ['use_aura==1 and OS=="win"', {
          'sources/': [
            ['exclude', 'dragdrop/drag_utils_aura.cc'],
          ],
        }],
        ['use_glib == 1', {
          'dependencies': [
            '../../build/linux/system.gyp:fontconfig',
            '../../build/linux/system.gyp:glib',
          ],
        }],
        ['desktop_linux == 1 or chromeos == 1', {
          'conditions': [
            ['toolkit_views==0 and use_aura==0', {
              # Note: because of gyp predence rules this has to be defined as
              # 'sources/' rather than 'sources!'.
              'sources/': [
                ['exclude', '^dragdrop/drag_utils.cc'],
                ['exclude', '^dragdrop/drag_utils.h'],
              ],
            }, {
              'sources/': [
                ['include', '^dragdrop/os_exchange_data.cc'],
                ['include', '^dragdrop/os_exchange_data.h'],
                ['include', '^nine_image_painter_factory.cc'],
                ['include', '^nine_image_painter_factory.h'],
              ],
            }],
          ],
        }],
        ['use_pango==1', {
          'dependencies': [
            '../../build/linux/system.gyp:pangocairo',
          ],
        }],
        ['OS=="win" or use_clipboard_aurax11==1', {
          'sources!': [
            'clipboard/clipboard_aura.cc',
          ],
        }, {
          'sources!': [
            'clipboard/clipboard_aurax11.cc',
          ],
        }],
        ['chromeos==1 or (use_aura==1 and OS=="linux" and use_x11==0)', {
          'sources!': [
            'dragdrop/os_exchange_data_provider_aurax11.cc',
            'touch/touch_device.cc',
          ],
        }, {
          'sources!': [
            'dragdrop/os_exchange_data_provider_aura.cc',
            'dragdrop/os_exchange_data_provider_aura.h',
            'touch/touch_device_aurax11.cc',
          ],
        }],
        ['OS=="win"', {
          'sources!': [
            'touch/touch_device.cc',
          ],
          'include_dirs': [
            '../..',
            '../../third_party/wtl/include',
          ],
          # TODO(jschuh): C4267: http://crbug.com/167187 size_t -> int
          # C4324 is structure was padded due to __declspec(align()), which is
          # uninteresting.
          'msvs_disabled_warnings': [ 4267, 4324 ],
          'msvs_settings': {
            'VCLinkerTool': {
              'DelayLoadDLLs': [
                'd2d1.dll',
                'd3d10_1.dll',
                'dwmapi.dll',
              ],
              'AdditionalDependencies': [
                'd2d1.lib',
                'd3d10_1.lib',
                'dwmapi.lib',
              ],
            },
          },
          'link_settings': {
            'libraries': [
              '-limm32.lib',
              '-ld2d1.lib',
              '-ldwmapi.lib',
              '-loleacc.lib',
            ],
          },
        },{  # OS!="win"
          'conditions': [
            ['use_aura==0', {
              'sources!': [
                'view_prop.cc',
                'view_prop.h',
              ],
            }],
          ],
        }],
        ['OS=="mac"', {
          'dependencies': [
            '../../third_party/mozilla/mozilla.gyp:mozilla',
          ],
          'sources!': [
            'cursor/image_cursors.cc',
            'cursor/image_cursors.h',
            'dragdrop/drag_utils.cc',
            'dragdrop/drag_utils.h',
          ],
          'link_settings': {
            'libraries': [
              '$(SDKROOT)/System/Library/Frameworks/Accelerate.framework',
              '$(SDKROOT)/System/Library/Frameworks/AudioUnit.framework',
              '$(SDKROOT)/System/Library/Frameworks/CoreVideo.framework',
            ],
          },
        }],
        ['use_x11==1', {
          'all_dependent_settings': {
            'ldflags': [
              '-L<(PRODUCT_DIR)',
            ],
          },
          'dependencies': [
            '../../build/linux/system.gyp:x11',
            '../../build/linux/system.gyp:xcursor',
            '../../build/linux/system.gyp:xext',
            '../../build/linux/system.gyp:xfixes',
            '../../build/linux/system.gyp:xrender',  # For XRender* function calls in x11_util.cc.
            '../events/platform/x11/x11_events_platform.gyp:x11_events_platform',
          ],
        }],
        ['toolkit_views==0', {
          'sources!': [
            'dragdrop/drag_drop_types.h',
            'dragdrop/drop_target_event.cc',
            'dragdrop/drop_target_event.h',
            'dragdrop/os_exchange_data.cc',
            'dragdrop/os_exchange_data.h',
            'nine_image_painter_factory.cc',
            'nine_image_painter_factory.h',
          ],
        }],
        ['OS=="android"', {
          'sources!': [
            'cursor/image_cursors.cc',
            'cursor/image_cursors.h',
            'default_theme_provider.cc',
            'dragdrop/drag_utils.cc',
            'dragdrop/drag_utils.h',
            'l10n/l10n_font_util.cc',
            'models/button_menu_item_model.cc',
            'models/dialog_model.cc',
            'theme_provider.cc',
            'touch/touch_device.cc',
            'touch/touch_editing_controller.cc',
            'ui_base_types.cc',
          ],
          'dependencies': [
            'ui_base_jni_headers',
          ],
          'link_settings': {
            'libraries': [
              '-ljnigraphics',
            ],
          },
        }],
        ['OS=="android" and android_webview_build==0', {
          'dependencies': [
            '../android/ui_android.gyp:ui_java',
          ],
        }],
        ['OS=="android" and use_aura==0', {
          'sources!': [
            'cursor/cursor_android.cc'
          ],
        }],
        ['OS=="android" and use_aura==1', {
          'sources!': [
            'clipboard/clipboard_aura.cc'
          ],
        }],
        ['OS=="android" or OS=="ios"', {
          'sources!': [
            'device_form_factor_desktop.cc'
          ],
        }],
        ['OS=="linux"', {
          'libraries': [
            '-ldl',
          ],
        }],
        ['use_system_icu==1', {
          # When using the system icu, the icu targets generate shim headers
          # which are included by public headers in the ui target, so we need
          # ui to be a hard dependency for all its users.
          'hard_dependency': 1,
        }],
        ['toolkit_views==0 and use_aura==0', {
          'sources!': [
            'ime/input_method_factory.cc',
            'ime/input_method_factory.h',
            'ime/input_method_minimal.cc',
            'ime/input_method_minimal.h',
          ],
        }],
        ['chromeos==0 or use_x11==0', {
          'sources!': [
            'ime/input_method_chromeos.cc',
            'ime/input_method_chromeos.h',
          ],
        }],
        ['chromeos==1', {
          'dependencies': [
            '../../chromeos/chromeos.gyp:chromeos',
          ],
        }],
        ['OS!="win"', {
          'sources!': [
            'ime/input_method_imm32.cc',
            'ime/input_method_imm32.h',
          ],
        }],
        ['use_aura==0 or (desktop_linux==0 and use_ozone==0)', {
          'sources!': [
            'ime/input_method_auralinux.cc',
            'ime/input_method_auralinux.h',
            'ime/linux/fake_input_method_context.cc',
            'ime/linux/fake_input_method_context.h',
            'ime/linux/fake_input_method_context_factory.cc',
            'ime/linux/fake_input_method_context_factory.h',
            'ime/linux/linux_input_method_context.h',
            'ime/linux/linux_input_method_context_factory.cc',
            'ime/linux/linux_input_method_context_factory.h',
          ],
        }],
        ['use_x11==0', {
          'sources!': [
            'ime/composition_text_util_pango.cc',
            'ime/composition_text_util_pango.h',
            'ime/chromeos/character_composer.cc',
            'ime/chromeos/character_composer.h',
          ],
        }],
      ],
    },
    {
      # GN version: //ui/base:test_support
      'target_name': 'ui_base_test_support',
      'dependencies': [
        '../../base/base.gyp:base',
        '../../skia/skia.gyp:skia',
        '../../testing/gtest.gyp:gtest',
        '../gfx/gfx.gyp:gfx',
        '../gfx/gfx.gyp:gfx_geometry',
      ],
      'sources': [
        # Note: file list duplicated in GN build.
        'test/ui_controls.h',
        'test/ui_controls_aura.cc',
        'test/ui_controls_internal_win.cc',
        'test/ui_controls_internal_win.h',
        'test/ui_controls_mac.mm',
        'test/ui_controls_win.cc',
      ],
      'include_dirs': [
        '../..',
      ],
      'conditions': [
        ['OS!="ios"', {
          'type': 'static_library',
            'sources': [
              'ime/dummy_input_method.cc',
              'ime/dummy_input_method.h',
              'ime/dummy_text_input_client.cc',
              'ime/dummy_text_input_client.h',
            ],
        }, {  # OS=="ios"
          # None of the sources in this target are built on iOS, resulting in
          # link errors when building targets that depend on this target
          # because the static library isn't found. If this target is changed
          # to have sources that are built on iOS, the target should be changed
          # to be of type static_library on all platforms.
          'type': 'none',
        }],
        ['use_aura==1', {
          'sources!': [
            'test/ui_controls_mac.mm',
            'test/ui_controls_win.cc',
          ],
        }],
      ],
    },
  ],
  'conditions': [
    ['OS=="android"' , {
       'targets': [
         {
           # GN version: //ui/base:ui_base_jni_headers
           'target_name': 'ui_base_jni_headers',
           'type': 'none',
           'sources': [
             # Note: file list duplicated in GN build.
             '../android/java/src/org/chromium/ui/base/Clipboard.java',
             '../android/java/src/org/chromium/ui/base/DeviceFormFactor.java',
             '../android/java/src/org/chromium/ui/base/LocalizationUtils.java',
             '../android/java/src/org/chromium/ui/base/ResourceBundle.java',
             '../android/java/src/org/chromium/ui/base/SelectFileDialog.java',
             '../android/java/src/org/chromium/ui/base/TouchDevice.java',
             '../android/java/src/org/chromium/ui/base/ViewAndroid.java',
             '../android/java/src/org/chromium/ui/base/WindowAndroid.java',
           ],
           'variables': {
             'jni_gen_package': 'ui',
           },
           'includes': [ '../../build/jni_generator.gypi' ],
         },
       ],
    }],
  ],
}
