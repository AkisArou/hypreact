#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <libcss/libcss.h>

typedef struct adapter_node {
    struct adapter_node *parent;
    lwc_string *type_name;
} adapter_node;

static css_error resolve_url(void *pw, const char *base, lwc_string *rel, lwc_string **abs)
{
    (void)pw;
    (void)base;
    *abs = lwc_string_ref(rel);
    return CSS_OK;
}

static css_error node_name(void *pw, void *node, css_qname *qname)
{
    (void)pw;
    adapter_node *adapter = (adapter_node *) node;
    qname->ns = NULL;
    qname->name = lwc_string_ref(adapter->type_name);
    return CSS_OK;
}

static css_error node_classes(void *pw, void *node, lwc_string ***classes, uint32_t *n_classes)
{
    (void)pw;
    (void)node;
    *classes = NULL;
    *n_classes = 0;
    return CSS_OK;
}

static css_error node_id(void *pw, void *node, lwc_string **id)
{
    (void)pw;
    (void)node;
    *id = NULL;
    return CSS_OK;
}

static css_error parent_node(void *pw, void *node, void **parent)
{
    (void)pw;
    *parent = ((adapter_node *) node)->parent;
    return CSS_OK;
}

static css_error no_named_node(void *pw, void *node, const css_qname *qname, void **out)
{
    (void)pw;
    (void)node;
    (void)qname;
    *out = NULL;
    return CSS_OK;
}

static css_error no_node(void *pw, void *node, void **out)
{
    (void)pw;
    (void)node;
    *out = NULL;
    return CSS_OK;
}

static css_error node_has_name(void *pw, void *node, const css_qname *qname, bool *match)
{
    (void)pw;
    adapter_node *adapter = (adapter_node *) node;
    return lwc_string_caseless_isequal(adapter->type_name, qname->name, match) == lwc_error_ok ? CSS_OK : CSS_NOMEM;
}

static css_error no_name_test(void *pw, void *node, lwc_string *name, bool *match)
{
    (void)pw;
    (void)node;
    (void)name;
    *match = false;
    return CSS_OK;
}

static css_error no_attr_test(void *pw, void *node, const css_qname *qname, bool *match)
{
    (void)pw;
    (void)node;
    (void)qname;
    *match = false;
    return CSS_OK;
}

static css_error no_attr_equal_test(void *pw, void *node, const css_qname *qname, lwc_string *value, bool *match)
{
    (void)pw;
    (void)node;
    (void)qname;
    (void)value;
    *match = false;
    return CSS_OK;
}

static css_error no_bool(void *pw, void *node, bool *match)
{
    (void)pw;
    (void)node;
    *match = false;
    return CSS_OK;
}

static css_error node_count_siblings(void *pw, void *node, bool same_name, bool after, int32_t *count)
{
    (void)pw;
    (void)node;
    (void)same_name;
    (void)after;
    *count = 1;
    return CSS_OK;
}

static css_error node_is_lang(void *pw, void *node, lwc_string *lang, bool *match)
{
    (void)pw;
    (void)node;
    (void)lang;
    *match = false;
    return CSS_OK;
}

static css_error node_presentational_hint(void *pw, void *node, uint32_t *nhints, css_hint **hints)
{
    (void)pw;
    (void)node;
    *nhints = 0;
    *hints = NULL;
    return CSS_OK;
}

static css_error ua_default_for_property(void *pw, uint32_t property, css_hint *hint)
{
    (void)pw;
    if (property == CSS_PROP_COLOR) {
        hint->data.color = 0x00000000;
        hint->status = CSS_COLOR_COLOR;
        return CSS_OK;
    }
    if (property == CSS_PROP_FONT_FAMILY) {
        hint->data.strings = NULL;
        hint->status = CSS_FONT_FAMILY_SANS_SERIF;
        return CSS_OK;
    }
    if (property == CSS_PROP_QUOTES) {
        hint->data.strings = NULL;
        hint->status = CSS_QUOTES_NONE;
        return CSS_OK;
    }
    if (property == CSS_PROP_VOICE_FAMILY) {
        hint->data.strings = NULL;
        hint->status = 0;
        return CSS_OK;
    }
    return CSS_INVALID;
}

static css_error set_libcss_node_data(void *pw, void *n, void *libcss_node_data)
{
    (void)pw;
    (void)n;
    (void)libcss_node_data;
    return CSS_OK;
}

static css_error get_libcss_node_data(void *pw, void *n, void **libcss_node_data)
{
    (void)pw;
    (void)n;
    *libcss_node_data = NULL;
    return CSS_OK;
}

int main(void)
{
    const char css[] = "window { display: none; }";
    css_stylesheet *sheet = NULL;
    css_select_ctx *ctx = NULL;
    css_select_results *results = NULL;
    lwc_string *window_name = NULL;
    adapter_node node = {0};
    css_media media = {.type = CSS_MEDIA_SCREEN};
    css_unit_ctx unit_ctx = {
        .viewport_width = 800 * (1 << CSS_RADIX_POINT),
        .viewport_height = 600 * (1 << CSS_RADIX_POINT),
        .font_size_default = 16 * (1 << CSS_RADIX_POINT),
        .font_size_minimum = 6 * (1 << CSS_RADIX_POINT),
        .device_dpi = 96 * (1 << CSS_RADIX_POINT),
        .root_style = NULL,
        .pw = NULL,
        .measure = NULL,
    };
    css_select_handler handler = {
        CSS_SELECT_HANDLER_VERSION_1,
        node_name,
        node_classes,
        node_id,
        no_named_node,
        no_named_node,
        no_named_node,
        no_named_node,
        parent_node,
        no_node,
        node_has_name,
        no_name_test,
        no_name_test,
        no_attr_test,
        no_attr_equal_test,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        no_bool,
        node_count_siblings,
        no_bool,
        no_bool,
        no_bool,
        no_bool,
        no_bool,
        no_bool,
        no_bool,
        no_bool,
        no_bool,
        no_bool,
        node_is_lang,
        node_presentational_hint,
        ua_default_for_property,
        set_libcss_node_data,
        get_libcss_node_data,
    };
    css_stylesheet_params params = {
        .params_version = CSS_STYLESHEET_PARAMS_VERSION_1,
        .level = CSS_LEVEL_21,
        .charset = "UTF-8",
        .url = "hypreact://c-adapter-repro",
        .title = "c-adapter-repro",
        .allow_quirks = false,
        .inline_style = false,
        .resolve = resolve_url,
        .resolve_pw = NULL,
        .import = NULL,
        .import_pw = NULL,
        .color = NULL,
        .color_pw = NULL,
        .font = NULL,
        .font_pw = NULL,
    };

    css_error error = css_stylesheet_create(&params, &sheet);
    if (error != CSS_OK) {
        fprintf(stderr, "create failed: %s\n", css_error_to_string(error));
        return 1;
    }

    error = css_stylesheet_append_data(sheet, (const uint8_t *) css, sizeof(css) - 1);
    if (error != CSS_OK && error != CSS_NEEDDATA) {
        fprintf(stderr, "append failed: %s\n", css_error_to_string(error));
        return 1;
    }

    error = css_stylesheet_data_done(sheet);
    if (error != CSS_OK) {
        fprintf(stderr, "done failed: %s\n", css_error_to_string(error));
        return 1;
    }

    error = css_select_ctx_create(&ctx);
    if (error != CSS_OK) {
        fprintf(stderr, "ctx create failed: %s\n", css_error_to_string(error));
        return 1;
    }

    error = css_select_ctx_append_sheet(ctx, sheet, CSS_ORIGIN_AUTHOR, NULL);
    if (error != CSS_OK) {
        fprintf(stderr, "append sheet failed: %s\n", css_error_to_string(error));
        return 1;
    }

    if (lwc_intern_string("window", 6, &window_name) != lwc_error_ok) {
        fprintf(stderr, "failed to intern node name\n");
        return 1;
    }
    node.type_name = window_name;

    error = css_select_style(ctx, &node, &unit_ctx, &media, NULL, &handler, NULL, &results);
    if (error != CSS_OK) {
        fprintf(stderr, "select failed: %s\n", css_error_to_string(error));
        return 1;
    }

    printf("ok\n");

    css_select_results_destroy(results);
    lwc_string_unref(window_name);
    css_select_ctx_destroy(ctx);
    css_stylesheet_destroy(sheet);
    return 0;
}
