// Included by octree

/**\typedef template<typename T_,typename R_,typename P_,typename O_,typename OP_,int d_> \
  *         typedef O_ octree_path<T_,R_,P_,O_,OP_,d_>::octree_type;
  * \brief Shorthand for an octree over which this class can iterate.
  */

/**\typedef template<typename T_,typename R_,typename P_,typename O_,typename OP_,int d_> \
  *         typedef OP_ octree_path<T_,R_,P_,O_,OP_,d_>::octree_pointer;
  * \brief Shorthand for a pointer to an octree over which this class can iterate.
  */

/**\typedef template<typename T_,typename R_,typename P_,typename O_,typename OP_,int d_> \
  *         typedef typename O_::allocator_type octree_path<T_,R_,P_,O_,OP_,d_>::octree_allocator_type;
  * \brief Shorthand for the allocator used by the octrees over which this class iterates.
  */

/**\typedef template<typename T_,typename R_,typename P_,typename O_,typename OP_,int d_> \
  *         typedef typename O_::octree_node_reference octree_path<T_,R_,P_,O_,OP_,d_>::octree_node_reference;
  * \brief Shorthand for a reference to a node in the octree.
  */

/**\typedef template<typename T_,typename R_,typename P_,typename O_,typename OP_,int d_> \
  *         typedef typename O_::octree_node_pointer octree_path<T_,R_,P_,O_,OP_,d_>::octree_node_pointer;
  * \brief Shorthand for a pointer to a node in the octree.
  */

/**\typedef template<typename T_,typename R_,typename P_,typename O_,typename OP_,int d_> \
  *         typedef octree_path< T_, T_&, T_*, O_, O_*, d_ > octree_path<T_,R_,P_,O_,OP_,d_>::path;
  * \brief Shorthand for a non-const octree path (regardless of whether the current path is const or not).
  */

/**\typedef template<typename T_,typename R_,typename P_,typename O_,typename OP_,int d_> \
  *         typedef octree_path< T_, const T_&, const T_*, O_, const O_*, d_ > octree_path<T_,R_,P_,O_,OP_,d_>::const_path;
  * \brief Shorthand for a const octree path (regardless of whether the current path is const or not).
  */

/**\typedef template<typename T_,typename R_,typename P_,typename O_,typename OP_,int d_> \
  *         typedef octree_path< T_, R_, P_, O_, OP_, d_ > octree_path<T_,R_,P_,O_,OP_,d_>::self_path;
  * \brief Shorthand for an path of the same type as the current path (be it const or not).
  */

/**\var template<typename T_,typename R_,typename P_,typename O_,typename OP_,int d_> \
  *     octree_pointer octree_path<T_,R_,P_,O_,OP_,d_>::_M_octree
  *\brief The octree we are iterating over
  */

/**\var template<typename T_,typename R_,typename P_,typename O_,typename OP_,int d_> \
  *     vtkstd::vector<octree_node_pointer> octree_path<T_,R_,P_,O_,OP_,d_>::_M_parents
  *\brief List of parent nodes
  */

/**\var template<typename T_,typename R_,typename P_,typename O_,typename OP_,int d_> \
  *     vtkstd::vector<int> octree_path<T_,R_,P_,O_,OP_,d_>::_M_indices
  *\brief List of parent child indices
  */

/**\var template<typename T_,typename R_,typename P_,typename O_,typename OP_,int d_> \
  *     octree_node_pointer octree_path<T_,R_,P_,O_,OP_,d_>::_M_current_node
  *\brief Current node at the head of the path
  */

/**\brief Default constructor.
  */
template< typename T_, typename R_, typename P_, typename O_, typename OP_, int d_ >
octree_path<T_,R_,P_,O_,OP_,d_>::octree_path()
{
  this->_M_octree = 0;
  this->_M_current_node = 0;
}

/**\brief Simplest valid constructor.
  *
  * This creates a path that points to the root node of the specified \a tree.
  */
template< typename T_, typename R_, typename P_, typename O_, typename OP_, int d_ >
octree_path<T_,R_,P_,O_,OP_,d_>::octree_path( octree_pointer tree )
{
  this->_M_octree = tree;
  this->_M_current_node = tree->root();
}

/**\brief Flexible constructor.
  *
  * This creates a path that points to a particular node of the specified \a tree,
  * given a \a path of nodes to descend from the root of the \a tree.
  */
template< typename T_, typename R_, typename P_, typename O_, typename OP_, int d_ >
octree_path<T_,R_,P_,O_,OP_,d_>::octree_path( octree_pointer tree, vtkstd::vector<int>& children )
{
  this->_M_octree = tree;
  this->_M_current_node = tree->root();
  for ( vtkstd::vector<int>::iterator cit = children.begin(); cit != children.end(); ++cit )
    {
    this->_M_parents.push_back( this->_M_current_node );
    this->_M_indices.push_back( *cit );
    this->_M_current_node = (*this->_M_current_node)[*cit];
    }
}

/**\brief Destructor.
  *
  */
template< typename T_, typename R_, typename P_, typename O_, typename OP_, int d_ >
octree_path<T_,R_,P_,O_,OP_,d_>::~octree_path()
{
}

/**\fn template< typename T_, typename R_, typename P_, typename O_, typename OP_, int d_ > \
  *    octree_node_reference octree_path<T_,R_,P_,O_,OP_,d_>::operator * () const
  *\brief Provide access to the node at the current path head.
  */

/**\fn template< typename T_, typename R_, typename P_, typename O_, typename OP_, int d_ > \
  *    octree_node_pointer octree_path<T_,R_,P_,O_,OP_,d_>::operator -> () const
  *\brief Provide access to the node at the current path head.
  */

/**\fn template< typename T_, typename R_, typename P_, typename O_, typename OP_, int d_ > \
  *    int octree_path<T_,R_,P_,O_,OP_,d_>::level() const
  *\brief Return the depth of the current path.
  *
  * The root node of the octree is at level 0. Its children are all at level 1. Their children are at level 2,
  * and so forth.
  */


/**\brief Assignment operator (for copying paths of mutable nodes).
  *
  */
template< typename T_, typename R_, typename P_, typename O_, typename OP_, int d_ >
octree_path<T_,R_,P_,O_,OP_,d_>& octree_path<T_,R_,P_,O_,OP_,d_>::operator = ( const path& src )
{
  this->_M_octree = src._M_octree;
  this->_M_parents = src._M_parents;
  this->_M_indices = src._M_indices;
  this->_M_current_node = src._M_current_node;
  return *this;
}

/**\fn    template< typename T_, typename R_, typename P_, typename O_, typename OP_, int d_ > \
  *       self_path& octree_path<T_,R_,P_,O_,OP_,d_>::operator = ( const const_path& src )
  *\brief Assignment operator (for copying paths of immutable nodes).
  * Frappy
  */

/**\brief Assignment operator (for copying paths of immutable nodes).
  *
  */
#if ! ( defined(_MSC_VER) && (_MSC_VER < 1300) )
template< typename T_, typename R_, typename P_, typename O_, typename OP_, int d_ >
octree_path<T_,R_,P_,O_,OP_,d_>& octree_path<T_,R_,P_,O_,OP_,d_>::operator = ( const const_path& src )
{
  this->_M_octree = const_cast<OP_>( src._M_octree );
  this->_M_parents = src._M_parents;
  this->_M_indices = src._M_indices;
  this->_M_current_node = src._M_current_node;
  return *this;
}
#endif

/**\fn template< typename T_, typename R_, typename P_, typename O_, typename OP_, int d_ > \
  *    bool octree_path<T_,R_,P_,O_,OP_,d_>::operator == ( const path& it )
  *\brief Compare two paths for equality.
  *
  * Iterators are considered equal iff they have the same octree and point to the same node currently.
  * The rest of the paths' state is ignored by the test.
  */

/**\fn template< typename T_, typename R_, typename P_, typename O_, typename OP_, int d_ > \
  *    bool octree_path<T_,R_,P_,O_,OP_,d_>::operator == ( const const_path& it )
  *\brief Compare two paths for equality.
  *
  * Iterators are considered equal iff they have the same octree and point to the same node currently.
  * The rest of the paths' state is ignored by the test.
  */

/**\fn template< typename T_, typename R_, typename P_, typename O_, typename OP_, int d_ > \
  *    bool octree_path<T_,R_,P_,O_,OP_,d_>::operator != ( const path& it )
  *\brief Compare two paths for inequality.
  *
  * Iterators are considered equal iff they have the same octree and point to the same node currently.
  * The rest of the paths' state is ignored by the test.
  */

/**\fn template< typename T_, typename R_, typename P_, typename O_, typename OP_, int d_ > \
  *    bool octree_path<T_,R_,P_,O_,OP_,d_>::operator != ( const const_path& it )
  *\brief Compare two paths for inequality.
  *
  * Iterators are considered equal iff they have the same octree and point to the same node currently.
  * The rest of the paths' state is ignored by the test.
  */
