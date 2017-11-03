// Copyright (c) 2016-2017, Intel Corporation.

// C includes
#include <stdlib.h>
#include <string.h>

#ifndef ZJS_LINUX_BUILD
// Zephyr includes
#include <zephyr.h>
#endif

#ifndef ZJS_LINUX_BUILD
// ZJS includes
#include "zjs_ipm.h"
#include "zjs_zephyr_port.h"
#else
#include "zjs_linux_port.h"
#endif

#include "zjs_callbacks.h"
#include "zjs_modules.h"
#include "zjs_modules_gen.h"
#include "zjs_script.h"
#include "zjs_timers.h"
#include "zjs_util.h"
#include "jerryscript-ext/module.h"

struct routine_map {
    zjs_service_routine func;
    void *handle;
};

static u8_t num_routines = 0;
struct routine_map svc_routine_map[NUM_SERVICE_ROUTINES];

static const jerryx_module_resolver_t *resolvers[] =
{
    &jerryx_module_native_resolver
};

static ZJS_DECL_FUNC(native_require_handler)
{
    // args: module name
    ZJS_VALIDATE_ARGS(Z_STRING);

    jerry_size_t size = MAX_MODULE_STR_LEN;
    char module[size];
    zjs_copy_jstring(argv[0], module, &size);
    if (!size) {
        return RANGE_ERROR("argument too long");
    }

    jerry_value_t native_module = jerryx_module_resolve(argv[0], resolvers, 1);
    // If we found our module, return it.
    if (jerry_value_has_error_flag(native_module)) {
        jerry_release_value(native_module);
    }
    else {
        return native_module;
    }
    DBG_PRINT("Native module not found, searching for JavaScript module %s\n",
              module);
#ifdef ZJS_LINUX_BUILD
    // Linux can pass in the script at runtime, so we have to read in/parse any
    // JS modules now rather than at compile time
    char full_path[size + 9];
    char *str;
    u32_t len;
    sprintf(full_path, "modules/%s", module);
    full_path[size + 8] = '\0';

    if (zjs_read_script(full_path, &str, &len)) {
        ERR_PRINT("could not read module %s\n", full_path);
        return NOTSUPPORTED_ERROR("could not read module script");
    }
    ZVAL code_eval = jerry_parse((jerry_char_t *)str, len, false);
    if (jerry_value_has_error_flag(code_eval)) {
        return SYSTEM_ERROR("could not parse javascript");
    }
    ZVAL result = jerry_run(code_eval);
    if (jerry_value_has_error_flag(result)) {
        return SYSTEM_ERROR("could not run javascript");
    }

    zjs_free_script(str);
#endif

    ZVAL global_obj = jerry_get_global_object();
    ZVAL modules_obj = zjs_get_property(global_obj, "module");

    if (!jerry_value_is_object(modules_obj)) {
        return SYSTEM_ERROR("modules object not found");
    }

    ZVAL exports_obj = zjs_get_property(modules_obj, "exports");
    if (!jerry_value_is_object(exports_obj)) {
        return SYSTEM_ERROR("exports object not found");
    }

    // TODO: maybe we should just error if no .js extension here
    char mod_trim[size];
    strncpy(mod_trim, module, size);
    if (size > 3 && !strncmp(mod_trim + size - 3, ".js", 3)) {
        // strip the ".js"
        mod_trim[size - 3] = '\0';
    }

    ZVAL found_obj = zjs_get_property(exports_obj, mod_trim);
    if (!jerry_value_is_object(found_obj)) {
        char err[80];
        snprintf(err, 80, "module not found: '%s'", module);
        return NOTSUPPORTED_ERROR(err);
    }

    DBG_PRINT("JavaScript module %s loaded\n", module);
    return jerry_acquire_value(found_obj);
}

// native eval handler
static ZJS_DECL_FUNC(native_eval_handler)
{
    return zjs_error("eval not supported");
}

// native print handler
static ZJS_DECL_FUNC(native_print_handler)
{
    if (argc < 1 || !jerry_value_is_string(argv[0]))
        return zjs_error("print: missing string argument");

    jerry_size_t size = 0;
    char *str = zjs_alloc_from_jstring(argv[0], &size);
    if (!str)
        return zjs_error("print: out of memory");

    ZJS_PRINT("%s\n", str);
    zjs_free(str);
    return ZJS_UNDEFINED;
}

static ZJS_DECL_FUNC(stop_js_handler)
{
#ifdef CONFIG_BOARD_ARDUINO_101
#ifdef CONFIG_IPM
    zjs_ipm_free_callbacks();
#endif
#endif
    zjs_modules_cleanup();
    jerry_cleanup();
    return ZJS_UNDEFINED;
}

#ifdef ZJS_LINUX_BUILD
static ZJS_DECL_FUNC(process_exit)
{
    ZJS_VALIDATE_ARGS(Z_OPTIONAL Z_NUMBER);

    int status = 0;

    if (argc > 0) {
        status = jerry_get_number_value(argv[0]);
    }
    ZJS_PRINT("Exiting with code=%d\n", status);
    exit(status);
}
#endif

void zjs_modules_init()
{
    // Add module.exports to global namespace
    ZVAL global_obj = jerry_get_global_object();
    ZVAL modules_obj = zjs_create_object();
    ZVAL exports_obj = zjs_create_object();

    zjs_set_property(modules_obj, "exports", exports_obj);
    zjs_set_property(global_obj, "module", modules_obj);

    // Todo: find a better solution to disable eval() in JerryScript.
    // For now, just inject our eval() function in the global space
    zjs_obj_add_function(global_obj, "eval", native_eval_handler);
    zjs_obj_add_function(global_obj, "print", native_print_handler);
    zjs_obj_add_function(global_obj, "stopJS", stop_js_handler);

    // create the C handler for require JS call
    zjs_obj_add_function(global_obj, "require", native_require_handler);

#ifdef ZJS_LINUX_BUILD
    ZVAL process = zjs_create_object();
    zjs_obj_add_function(process, "exit", process_exit);
    zjs_set_property(global_obj, "process", process);
#endif

    // initialize callbacks early in case any init functions use them
    zjs_init_callbacks();
    // Load global modules
    int gbl_modcount = sizeof(zjs_global_array) / sizeof(gbl_module_t);
    for (int i = 0; i < gbl_modcount; i++) {
        gbl_module_t *mod = &zjs_global_array[i];
        mod->init();
    }
    // initialize fixed modules
    zjs_error_init();
    zjs_timers_init();
}

void zjs_modules_cleanup()
{
    // stop timers first to prevent further calls
    zjs_timers_cleanup();
    zjs_unregister_service_routines();
    int gbl_modcount = sizeof(zjs_global_array) / sizeof(gbl_module_t);
    for (int i = 0; i < gbl_modcount; i++) {
        gbl_module_t *mod = &zjs_global_array[i];
        if (mod->cleanup) {
            mod->cleanup();
        }
    }
    // clean up fixed modules
    zjs_error_cleanup();

#ifdef ZJS_TRACE_MALLOC
    zjs_print_mem_stats();
#endif
}

void zjs_register_service_routine(void *handle, zjs_service_routine func)
{
    if (num_routines >= NUM_SERVICE_ROUTINES) {
        DBG_PRINT(("not enough space, increase NUM_SERVICE_ROUTINES\n"));
        return;
    }
    svc_routine_map[num_routines].handle = handle;
    svc_routine_map[num_routines].func = func;
    num_routines++;
    return;
}

void zjs_unregister_service_routines() {
    for (int i = 0; i < num_routines; i++) {
        //svc_routine_map[i].handle = NULL;
        // BJONES DOESN'T WORK yet
        // Try and set the pointers to null? then free the struct?
        // Also, sensor doesn't use this so can't be the problem there.
        // Ask Jimmy if there is anything similar in sensor, it might be  my culprit
        // Also also.... change this to be where the module unregister's itself
        // rather than doing it brute force.
        zjs_free(&svc_routine_map[i]);
    }
    num_routines = 0;
}

s32_t zjs_service_routines(void)
{
    s32_t wait = ZJS_TICKS_FOREVER;
    int i;
    for (i = 0; i < num_routines; ++i) {
        s32_t ret = svc_routine_map[i].func(svc_routine_map[i].handle);
        wait = (wait < ret) ? wait : ret;
    }
#ifdef ZJS_LINUX_BUILD
    if (wait == ZJS_TICKS_FOREVER) {
        return 0;
    }
#endif
    return wait;
}
