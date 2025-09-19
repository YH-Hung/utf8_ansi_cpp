# Functional Requirements

- Provide functions to converting std::string encoded between utf-8 and other ansi encodings, such as big5.
- Functions MUST be able to call standalone, no require to instantiate a class instance, not associated to a class.
- The main focus of another ansi encoding is big5, but user can specify other encoding names.
- User can also provide char* or char[] as input.

# Non-Functional Requirements

- The converting capability should be provided by ICU library. You can assume user had installed ICU on their system.
- Adopt modern C++ syntax, practices as mush as possible.
- Provide a readme.md under project root
  - Purpose of the project
  - Pre-requisites
  - How to build
  - How to use
  - API documentation