extern crate proc_macro;
use core::iter::FromIterator;
use proc_macro::{Group, TokenStream, TokenTree};

fn remove_attributes(item: TokenStream) -> TokenStream {
    let stream = item.into_iter();
    let mut is_attribute = false;
    let mut result = Vec::new();

    for next in stream {
        match next.clone() {
            TokenTree::Punct(p) => {
                if p.to_string() == "#" {
                    is_attribute = true;
                } else {
                    result.push(next.clone());
                }
            }
            TokenTree::Group(g) => {
                if is_attribute {
                    continue;
                } else {
                    let delimiter = g.delimiter();
                    let cleaned_group = remove_attributes(g.stream());
                    let cleaned_group = TokenTree::Group(Group::new(delimiter, cleaned_group));
                    result.push(cleaned_group);
                }
            }
            _ => {
                is_attribute = false;
                result.push(next.clone());
            }
        }
    }

    TokenStream::from_iter(result)
}

enum ParserState {
    Name,
    Type,
    //       open angle brackets
    Generics(usize),
}
fn parse_struct_fields(group: Vec<TokenTree>) -> Vec<ParsedField> {
    let mut fields = Vec::new();
    let mut field_ = ParsedField::new();
    let mut field_parser_state = ParserState::Name;
    for token in group {
        match (token, &field_parser_state) {
            (TokenTree::Ident(i), ParserState::Name) => {
                if i.to_string() == "pub" {
                    continue;
                } else {
                    field_.name = i.to_string();
                }
            }
            (TokenTree::Ident(i), ParserState::Type) => {
                field_.type_ = i.to_string();
            }
            (TokenTree::Ident(i), ParserState::Generics(_)) => {
                field_.generics = format!("{}{}", field_.generics, i.to_string());
            }
            (TokenTree::Punct(p), ParserState::Name) => {
                if p.to_string() == ":" {
                    field_parser_state = ParserState::Type
                } else {
                    panic!(
                        "Unexpected token '{}' in parsing {:#?}",
                        p.to_string(),
                        field_
                    );
                }
            }
            (TokenTree::Punct(p), ParserState::Type) => match p.to_string().as_ref() {
                "," => {
                    field_parser_state = ParserState::Name;
                    fields.push(field_.clone());
                    field_ = ParsedField::new();
                }
                "<" => {
                    field_.generics = "<".to_string();
                    field_parser_state = ParserState::Generics(0);
                }
                _ => panic!(
                    "Unexpected token '{}' in parsing {:#?}",
                    p.to_string(),
                    field_
                ),
            },
            (TokenTree::Punct(p), ParserState::Generics(open_brackets)) => {
                match p.to_string().as_ref() {
                    "'" => {
                        field_.generics = format!("{}{}", field_.generics, p.to_string());
                    }
                    "<" => {
                        field_.generics = format!("{}{}", field_.generics, p.to_string());
                        field_parser_state = ParserState::Generics(open_brackets + 1);
                    }
                    ">" => {
                        field_.generics = format!("{}{}", field_.generics, p.to_string());
                        if open_brackets == &0 {
                            field_parser_state = ParserState::Type
                        } else {
                            field_parser_state = ParserState::Generics(open_brackets - 1);
                        }
                    }
                    _ => {
                        field_.generics = format!("{}{}", field_.generics, p.to_string());
                    }
                }
            }
            _ => panic!("Unexpected token"),
        }
    }
    fields
}

#[derive(Clone, Debug)]
struct ParsedField {
    name: String,
    type_: String,
    generics: String,
}

impl ParsedField {
    pub fn new() -> Self {
        ParsedField {
            name: "".to_string(),
            type_: "".to_string(),
            generics: "".to_string(),
        }
    }

    pub fn get_generics(&self) -> String {
        if self.generics == "<'decoder>" || self.generics.is_empty() {
            "".to_string()
        } else {
            format!("::{}", self.generics.clone())
        }
    }
}

#[derive(Clone, Debug)]
struct ParsedStruct {
    pub name: String,
    pub generics: String,
    pub fields: Vec<ParsedField>,
}

// impl ParsedStruct {
//     pub fn new() -> Self {
//         ParsedStruct {
//             name: "".to_string(),
//             generics: "".to_string(),
//             fields: Vec::new(),
//         }
//     }
// }

fn get_struct_properties(item: TokenStream) -> ParsedStruct {
    let item = remove_attributes(item);
    let mut stream = item.into_iter();

    // Check if the stream is a struct
    loop {
        match stream.next().expect("Stream not a struct") {
            TokenTree::Ident(i) => {
                if i.to_string() == "struct" {
                    break;
                }
            }
            _ => continue,
        }
    }

    // Get the struct name
    let struct_name = match stream.next().expect("Struct has no name") {
        TokenTree::Ident(i) => i.to_string(),
        _ => panic!("Strcut has no name"),
    };

    let mut struct_generics = "".to_string();
    let group: Vec<TokenTree>;

    // Get the struct generics if any
    loop {
        match stream
            .next()
            .unwrap_or_else(|| panic!("Struct {} has no fields", struct_name))
        {
            TokenTree::Group(g) => {
                group = g.stream().into_iter().collect();
                break;
            }
            TokenTree::Punct(p) => {
                struct_generics = format!("{}{}", struct_generics, p.to_string());
            }
            TokenTree::Ident(i) => {
                struct_generics = format!("{}{}", struct_generics, i.to_string());
            }
            _ => panic!("Struct {} has no fields", struct_name),
        };
    }

    let fields = parse_struct_fields(group);

    ParsedStruct {
        name: struct_name,
        generics: struct_generics,
        fields,
    }
}

#[proc_macro_derive(Decodable)]
pub fn decodable(item: TokenStream) -> TokenStream {
    let parsed_struct = get_struct_properties(item);

    let mut derive_fields = String::new();

    for f in parsed_struct.fields.clone() {
        let field = format!(
            "
            let {}: Vec<FieldMarker> = {}{}::get_structure(& data[offset..])?;
            offset += {}.size_hint_(&data, offset)?;
            let {} =  {}.into();
            fields.push({});
            ",
            f.name,
            f.type_,
            f.get_generics(),
            f.name,
            f.name,
            f.name,
            f.name
        );
        derive_fields.push_str(&field)
    }

    let mut derive_decoded_fields = String::new();
    let mut fields = parsed_struct.fields.clone();

    // Reverse the fields as they are popped out from the end of the vector but we want to pop out
    // from the front
    fields.reverse();

    // Create Struct from fields
    for f in fields.clone() {
        let field = format!(
            "
            {}: {}{}::from_decoded_fields(data.pop().unwrap().into())?,
            ",
            f.name,
            f.type_,
            f.get_generics()
        );
        derive_decoded_fields.push_str(&field)
    }

    let impl_generics = if !parsed_struct.generics.is_empty() {
        parsed_struct.clone().generics
    } else {
        "<'decoder>".to_string()
    };

    let result = format!(
        "mod impl_parse_decodable_{} {{

    use super::binary_codec_sv2::{{decodable::DecodableField, decodable::FieldMarker, Decodable, Error, SizeHint}};
    use super::*;

    impl{} Decodable<'decoder> for {}{} {{
        fn get_structure(data: &[u8]) -> Result<Vec<FieldMarker>, Error> {{
            let mut fields = Vec::new();
            let mut offset = 0;
            {}
            Ok(fields)
        }}

        fn from_decoded_fields(mut data: Vec<DecodableField<'decoder>>) -> Result<Self, Error> {{
            Ok(Self {{
                {}
            }})
        }}
    }}
    }}",
        // imports
        parsed_struct.name.to_lowercase(),
        // derive decodable
        impl_generics,
        parsed_struct.name,
        parsed_struct.generics,
        derive_fields,
        derive_decoded_fields,
    );
    //println!("{}", result);

    result.parse().unwrap()
}

#[proc_macro_derive(Encodable)]
pub fn encodable(item: TokenStream) -> TokenStream {
    let parsed_struct = get_struct_properties(item);
    let fields = parsed_struct.fields.clone();

    let mut field_into_decoded_field = String::new();

    // Create DecodableField from fields
    for f in fields.clone() {
        let field = format!(
            "
            let val = v.{};
            fields.push(val.into());
            ",
            f.name
        );
        field_into_decoded_field.push_str(&field)
    }

    let mut sizes = String::new();

    for f in fields {
        let field = format!(
            "
            size += self.{}.get_size();
            ",
            f.name
        );
        sizes.push_str(&field)
    }
    let impl_generics = if !parsed_struct.generics.is_empty() {
        parsed_struct.clone().generics
    } else {
        "<'decoder>".to_string()
    };

    let result = format!(
        "mod impl_parse_encodable_{} {{

    use super::binary_codec_sv2::{{encodable::EncodableField, GetSize}};
    use super::{};
    extern crate alloc;
    use alloc::vec::Vec;

    impl{} From<{}{}> for EncodableField<'decoder> {{
        fn from(v: {}{}) -> Self {{
            let mut fields: Vec<EncodableField> = Vec::new();
            {}
            Self::Struct(fields)
        }}
    }}


    impl{} GetSize for {}{} {{
        fn get_size(&self) -> usize {{
            let mut size = 0;
            {}
            size
        }}
    }}

    }}",
        // imports
        parsed_struct.name.to_lowercase(),
        parsed_struct.name,
        // impl From<Struct> for DecodableField
        impl_generics,
        parsed_struct.name,
        parsed_struct.generics,
        parsed_struct.name,
        parsed_struct.generics,
        field_into_decoded_field,
        // impl Encodable for Struct
        //impl{} Encodable<'decoder> for {}{} {{}}
        //impl_generics,
        //parsed_struct.name,
        //parsed_struct.generics,
        // impl GetSize for Struct
        impl_generics,
        parsed_struct.name,
        parsed_struct.generics,
        sizes,
    );
    //println!("{}", result);

    result.parse().unwrap()
}
