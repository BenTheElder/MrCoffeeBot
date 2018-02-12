/*
Copyright 2016 Benjamin Elder (BenTheElder)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

/*
    A simple linked list implementation for use with DS1820 library
*/
#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <cstddef>

template<class T>
class Node
{
public:
    T* data;
    Node<T>* next;
};

template<class T>
class LinkedList
{
private:
    Node<T>* head;
public:
    LinkedList() {
        head = NULL;
    }

    ~LinkedList() {
        while (this->head != NULL) {
            this->remove(0);
        }
    }

    /*
    * peek returns the value at index, or NULL if the index is out of bounds
    */
    T* peek(int index) {
        int curr_index = 0;
        Node<T>* curr = this->head;
        while (curr != NULL && curr_index != index) {
            curr = curr->next;
        }
        if (curr_index == index) {
            return curr->data;
        }
        return NULL;
    }
    /*
    * remove returns the value at index after removing it from the list or
    * NULL if the index is out of bounds
    */
    T* remove(int index) {
        int curr_index = 0;
        Node<T>* curr = this->head;
        Node<T>* prev = NULL;
        if (curr == NULL) {
            return NULL;
        }
        while (curr != NULL && curr_index != index) {
            prev = curr;
            curr = curr->next;
        }
        if (curr_index == index) {
            T* data = curr->data;
            prev->next = curr->next;
            delete curr;
            return data;
        }
        return NULL;
    }

    /*
    * push inserts data at the beginning of the list
    */
    void push(T* data) {
        Node<T>* temp = this->head;
        this->head = new Node<T>;
        this->head->data = data;
        this->head->next = temp;
    }

    /*
    * append inserts data at the end of the list
    */
    void append(T* data) {
        Node<T>* temp = this->head;
        if (temp == NULL) {
            this->push(data);
            return;
        }
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = new Node<T>;
        temp->next->data = data;
        temp->next->next = NULL;
    }

    /*
    * length returns the number of elements currently in the list
    */
    int length() {
        Node<T>* curr = this->head;
        int count = 0;
        while (curr != NULL) {
            count += 1;
            curr = curr->next;
        }
        return count;
    }
};

#endif