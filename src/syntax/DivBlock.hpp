#pragma once

#include"../interfaces/ast.hpp"
#include"../interfaces/parse.hpp"
#include"../interfaces/syntax.hpp"
#include"../utils.hpp"

#include"Markdown.hpp"

namespace almo {

struct DivBlock : public ASTNode {
  private:
    std::string div_class;
public:
    DivBlock(std::string div_class) : div_class(div_class) {
        set_uuid();
    }

    std::string to_html() const override {
        return "<div class=\"" + div_class + "\">" + concatenated_childs_html() + "</div>";
    }

    std::map<std::string, std::string> get_properties() const override {
        return {
            {"div_class", div_class},
        };
    }
    std::string get_classname() const override {
        return "DivBlock";
    }
};

struct DivBlockSyntax : public BlockSyntax {
    bool operator()(Reader &read) const override {
        if (!read.is_line_begin()) return false;
        if (!read.get_row().starts_with(":::")) return false;
        if (rtrim(read.get_row()) != ":::") return true;
        return false;
    }
    void operator()(Reader &read, ASTNode &ast) const override {
        std::stack<std::shared_ptr<DivBlock>> scopes;
        std::string title = read.get_row().substr(3);

        DivBlock root_div(title);
        scopes.push(std::make_shared<DivBlock>(root_div));

        read.move_next_line();
        std::vector<std::string> text;

        while (!read.is_eof()){
            if (rtrim(read.get_row()) == ":::"){
                Markdown inner_md;
                MarkdownParser parser(text, read.meta_data);
                parser.process(inner_md);
                auto inner_md_ptr = std::make_shared<Markdown>(inner_md);
                scopes.top()->add_child(inner_md_ptr);

                text = {};
                read.move_next_line();

                if (scopes.size() == 1u){
                    ast.add_child(scopes.top());
                    break;
                }
                scopes.pop();
                continue;
            }
            if (read.get_row().starts_with(":::")){
                Markdown inner_md;
                MarkdownParser parser(text, read.meta_data);
                parser.process(inner_md);
                auto inner_md_ptr = std::make_shared<Markdown>(inner_md);
                scopes.top()->add_child(inner_md_ptr);

                DivBlock new_node(read.get_row().substr(3));
                auto new_node_ptr = std::make_shared<DivBlock>(new_node);
                scopes.top()->add_child(new_node_ptr);

                scopes.push(new_node_ptr);

                text = {};
                read.move_next_line();
                continue;
            }
            text.emplace_back(read.get_row());
            read.move_next_line();
        }
        while (true){
            Markdown inner_md;
            MarkdownParser parser(text, read.meta_data);
            parser.process(inner_md);
            auto inner_md_ptr = std::make_shared<Markdown>(inner_md);
            scopes.top()->add_child(inner_md_ptr);

            text = {};
            read.move_next_line();

            if (scopes.size() == 1u){
                ast.add_child(scopes.top());
                break;
            }
            scopes.pop();
            continue;
        }
    }
};

} // namespace almo